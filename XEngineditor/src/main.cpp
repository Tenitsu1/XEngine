#include "XEngine/engine.h"
#include "XEngine/app.h"
#include "XEngine/log.h"


#include "XEngine/shaders/computeShader.h"
#include "XEngine/shaders/shader.h"

#include "XEngine/graphics/gltfLoader.h"
#include "XEngine/graphics/structs.hpp"
#include "XEngine/graphics/camera.hpp"
#include "XEngine/graphics/cameraController.hpp"
#include "XEngine/graphics/Gbuffer.h"

#include "XEngine/input/mouse.h"
#include "XEngine/input/keyboard.h"

#include "XEngine/accelerators/obvh.h"

#include "external/imgui/imgui.h"
#include "external/glm/glm.hpp"
#include "external/glm/gtc/matrix_transform.hpp"
#include "external/tinygltf/tiny_gltf.h"

#include <string>
#include <filesystem>

using namespace XEngine;

class Editor : public XEngine::App
{

private:

	// Shader
	std::shared_ptr<ComputeShader> mComputeShader;
	std::shared_ptr<Shader> mShader;

	// Model
	tinygltf::Model mtinyModel;
	std::shared_ptr<graphics::GLTFStaticMesh> mModel;

	// Gbuffer
	std::shared_ptr<graphics::GBuffer> mGBuffer;
	std::shared_ptr<Shader> mGBufferShader;

	// Camera && Controller
	Camera camera;
	std::unique_ptr<CameraController> mCameraController;
	float xkeyOffset = 0.f;
	float ykeyOffset = 0.f;
	float zkeyOffset = 5.f;
	bool cameraUpdated = false;
	bool GuiCameraChanged = false;

	// SSBO
	GLuint mVerticesSSBO = 0; // vertex  SSBO-ID
	GLuint mIndicesSSBO = 0;  // indices SSBO-ID
	GLuint mNormalsSSBO = 0;  // normal  SSBO-ID
	GLuint mOBVHSSBO = 0;     // OBVH    SSBO-ID
	GLuint mMaterialDataSSBO = 0;
	GLuint mFaceNormalsSSBO = 0;
	GLuint mTexCoordsSSBO = 0;
	GLuint mMaterialIndicesSSBO = 0;
	GLuint mMatToTexMapSSBO = 0;

	GLuint mScreenTextures[2] = {0, 0};
	int mCurrentFrame = 0;

	uint64_t nowTime = Engine::Instance().getWindow().getDeltaTime();
	uint64_t laseTime = 0;
	float deltaTime = 0;
	int triangleCount = 0; 

	int samples_per_pixel = 1;
	int max_depth = 5;

	float lastX = 640, lastY = 450;
	bool useOBVH = true;

public:

	core::WindowProperties getWindowProperties()
	{
		core::WindowProperties props;
		props.title = "RayTracing Project";
		props.width = 1600;
		props.height = 900;
		props.imguiProps.isViewportEnable = true;
		props.imguiProps.isDockingEnable = true;
		return props;
	}

	void initialize() override
	{
		int width = getWindowProperties().width;
		int height = getWindowProperties().height;

		mModel = std::make_shared<graphics::GLTFStaticMesh>(mtinyModel, "models\\cornell_box\\CornellBox_Metallic.gltf");
		auto& Mesh = mModel->getMesh();

		std::vector<Material> materials = mModel->getMaterials();

		mGBuffer = std::make_shared<graphics::GBuffer>();
		if (!mGBuffer->initialize(width, height)) XENGINE_ERROR("Failed to initialize GBuffer!");
		mGBufferShader = std::make_shared<Shader>("shaders/gbuffer.vert", "shaders/gbuffer.frag");



		triangleCount = (int)Mesh.indices.size();
		mShader = std::make_shared<Shader>("shaders\\default.vert", "shaders\\default.frag");
		mShader->createTexture(width, height);
		mShader->setFBO();
		mShader->setVAO();
		mShader->setVBO();
		float quadVertices[] = {
			// positions   // texCoords
			-1.0f, -1.0f,   0.0f, 0.0f,
			 1.0f, -1.0f,   1.0f, 0.0f,
			 1.0f,  1.0f,   1.0f, 1.0f,
			-1.0f,  1.0f,   0.0f, 1.0f
		};
		unsigned int quadIndices[] = { 0, 1, 2, 0, 2, 3 };
		mShader->setEBO(quadIndices, sizeof(quadIndices));
		mShader->bind(quadVertices, 4, 4);
		mComputeShader = std::make_shared<ComputeShader>("shaders\\default.comp", width, height);
		mScreenTextures[0] = mComputeShader->createTexture(width, height);
		mScreenTextures[1] = mComputeShader->createTexture(width, height);

		auto obvhNodes = OBVH::buildOBVH(Mesh);


		if (triangleCount > 0)
		{
			mComputeShader->createSSBO(mOBVHSSBO,         (uint32_t)obvhNodes.size()         * sizeof(OBVH::OBVHNode), obvhNodes.data(),         2);
			mComputeShader->createSSBO(mVerticesSSBO,     (uint32_t)Mesh.vertices.size()     * sizeof(glm::vec4),      Mesh.vertices.data(),     3);	
			mComputeShader->createSSBO(mIndicesSSBO,      (uint32_t)Mesh.indices.size()      * sizeof(glm::ivec4),     Mesh.indices.data(),      4);
			mComputeShader->createSSBO(mFaceNormalsSSBO,  (uint32_t)Mesh.faceNormals.size()  * sizeof(glm::vec4),      Mesh.faceNormals.data(),  5);
			mComputeShader->createSSBO(mNormalsSSBO,      (uint32_t)Mesh.normals.size()      * sizeof(glm::vec4),      Mesh.normals.data(),      9);

			if (!Mesh.texCoords.empty()) 
			{
				mComputeShader->createSSBO(mTexCoordsSSBO, (uint32_t)Mesh.texCoords.size() * sizeof(glm::vec2), Mesh.texCoords.data(), 6);
			}
			else 
			{
				XENGINE_WARN("Model has no texture coordinates!");
			}

			// [修改] Material Indices (Binding 7)
			if (!Mesh.materialIndices.empty()) 
			{
				mComputeShader->createSSBO(mMaterialIndicesSSBO,
					(uint32_t)Mesh.materialIndices.size() * sizeof(int),
					Mesh.materialIndices.data(), 7);
			}

			// [新增] Material Data SSBO (Binding 8)
			// 將整個 GPUMaterial 陣列傳入 GPU
			if (!materials.empty()) 
			{
				mComputeShader->createSSBO(mMaterialDataSSBO,
					(uint32_t)materials.size() * sizeof(Material),
					materials.data(), 8);
			}
			
			 {
				XENGINE_TRACE("Vertices SSBO created, size: {}, count: {}",
					(uint32_t)Mesh.vertices.size() * sizeof(glm::vec4), Mesh.vertices.size());
				XENGINE_TRACE("Indices SSBO created, size: {}, count: {}",
					(uint32_t)Mesh.indices.size() * sizeof(glm::ivec4), Mesh.indices.size());
				XENGINE_TRACE("Normals SSBO created, size: {}, count: {}",
					(uint32_t)Mesh.normals.size() * sizeof(glm::vec4), Mesh.normals.size());
				XENGINE_TRACE("Normals SSBO created, size: {}, count: {}",
					(uint32_t)Mesh.faceNormals.size() * sizeof(glm::vec4), Mesh.faceNormals.size());
				XENGINE_TRACE("TexCoords SSBO created, size: {}, count: {}",
					(uint32_t)Mesh.texCoords.size() * sizeof(glm::vec2), Mesh.texCoords.size());
				XENGINE_TRACE("MaterialIndices SSBO created, size: {}, count: {}",
					(uint32_t)Mesh.materialIndices.size() * sizeof(int), Mesh.materialIndices.size());
				
			} 
		}



		// --- 印出包圍盒日誌 ---
		glm::vec3 boundsMin = mModel->getBoundsMin();
		glm::vec3 boundsMax = mModel->getBoundsMax();
		glm::vec3 center = (boundsMin + boundsMax) * 0.5f;
		glm::vec3 size = boundsMax - boundsMin;

		XENGINE_TRACE("=========================================");
		XENGINE_TRACE("Model Bounding Box Info:");
		XENGINE_TRACE("  Min: ({:.2f}, {:.2f}, {:.2f})", boundsMin.x, boundsMin.y, boundsMin.z);
		XENGINE_TRACE("  Max: ({:.2f}, {:.2f}, {:.2f})", boundsMax.x, boundsMax.y, boundsMax.z);
		XENGINE_TRACE("  Center: ({:.2f}, {:.2f}, {:.2f})", center.x, center.y, center.z);
		XENGINE_TRACE("  Size: ({:.2f}, {:.2f}, {:.2f})", size.x, size.y, size.z);
		XENGINE_TRACE("=========================================");



		camera = Camera(glm::vec3(8.f, 4.0f, 0.2f));
		camera.Yaw = -180.0f;
		camera.Pitch = 0.0f;
		camera.MovementSpeed = 5.0f; 
		camera.ProcessMouseMovement(0, 0);

		mCameraController = std::make_unique<CameraController>(camera);
		mCameraController->SetSpeed(10.0f);

	}
	void shutdown() override
	{
		
	}
	void update() override
	{
		auto windowSize = Engine::Instance().getWindow().getWindowSize();

		// Calculate the delta time
		laseTime = nowTime;
		nowTime = Engine::Instance().getWindow().getDeltaTime();
		deltaTime = (float)((nowTime - laseTime) * 1000 / (float)Engine::Instance().getWindow().getDeltaTime());

		float timeStep = deltaTime * 10.0f;


		// Camera Updata
		cameraUpdated = mCameraController->OnUpdate(timeStep);

		if (GuiCameraChanged)
		{
			cameraUpdated = true;     
			GuiCameraChanged = false;
		}


		//// 計算矩陣
		//glm::mat4 view = camera.GetViewMatrix();
		//glm::mat4 projection = camera.GetProjectionMatrix((float)windowSize.x, (float)windowSize.y);
		//glm::mat4 invViewProj = glm::inverse(projection * view);
		//CameraData cameraShaderData = camera.GetShaderData();

		//// Compute Shader setup
		//mComputeShader->bind();
		//mComputeShader->setUniformFloat2("u_resolution", (float)windowSize.x, (float)windowSize.y);
		//mComputeShader->setUniformFloat1("u_time", (float)nowTime / 1000.0f);
		//mComputeShader->setUniformCamera("camera", cameraShaderData);
		//mComputeShader->setUniformBool("cameraUpdated", cameraUpdated);
		//mComputeShader->setUniformMat4("invViewProj", invViewProj);
		//mComputeShader->setUniformInt("SAMPLES_PER_PIXEL", samples_per_pixel);
		//mComputeShader->setUniformInt("MAX_DEPTH", max_depth);
		//mComputeShader->setUniformFloat3("backgroundColor", 0.5f, 0.5f, 0.5f);
		//mComputeShader->setUniformBool("useOBVH", useOBVH);

		//mCurrentFrame = 1 - mCurrentFrame;
		//GLuint writeTex = mScreenTextures[mCurrentFrame];
		//GLuint readTex  = mScreenTextures[1 - mCurrentFrame];

		//// 綁定 image2D 作為輸出
		//mComputeShader->bindImageTexture(writeTex, 0);
		//// 綁定上一偵作為 sampler2D 輸入
		//mComputeShader->bindTexture(readTex, 1); // 1 = prevFrameTexture 的 binding

		//const auto& textures = mModel->getTextures();
		//if (!textures.empty())
		//{
		//	// 最多綁定 32 個紋理
		//	int max_textures_to_bind = std::min((int)textures.size(), 32);

		//	for (int i = 0; i < max_textures_to_bind; ++i)
		//	{
		//		// 將紋理綁定到紋理單元 10 + i 
		//		int textureUnit = 10 + i;
		//		mComputeShader->bindTexture(textures[i], textureUnit);
		//	}

		//	mComputeShader->setUniformInt("u_texture_count", max_textures_to_bind);
		//}
	}

	void render() override
	{
		int width = getWindowProperties().width;
		int height = getWindowProperties().height;

		glm::mat4 view = camera.GetViewMatrix();
		glm::mat4 projection = camera.GetProjectionMatrix((float)width, (float)height);

		// =============================================================
		// Phase 1: Geometry Pass (Rasterization -> GBuffer)
		// =============================================================
		{
			// 1. 綁定 GBuffer 寫入
			mGBuffer->bindForWriting();

			// 2. 設定狀態 & 清除
			//glEnable(GL_DEPTH_TEST);
			//// 背景色設為 0 (alpha=0)，Compute Shader 讀到 0 會視為背景/天空
			//glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
			//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			// 3. 啟用光柵化 Shader
			mGBufferShader->bind(); // 這裡使用修改後的無參數 bind()

			// 設定矩陣
			mGBufferShader->setUniformMat4("view", view);
			mGBufferShader->setUniformMat4("projection", projection);
			mGBufferShader->setUniformMat4("model", glm::mat4(1.0f));

			// 重要：確保紋理單元正確
			// 這裡假設 gbuffer.frag 中的 sampler 綁定為:
			// texture_baseColor -> 0, texture_metallicRoughness -> 1, etc.
			// 你可以在 Shader::bind 或初始化時設定這些 uniform int，或者在這裡設
			mGBufferShader->setUniformInt("texture_baseColor", 0);
			mGBufferShader->setUniformInt("texture_metallicRoughness", 1);
			mGBufferShader->setUniformInt("texture_normal", 2);
			mGBufferShader->setUniformInt("texture_emissive", 3);

			// 4. 繪製場景
			// 注意：這個 drawModel 會呼叫 glDrawElements
			// 同時它需要負責綁定每個 SubMesh 對應的材質紋理到 slot 0, 1, 2, 3
			// 如果你的 drawModel 沒有綁定紋理的邏輯，GBuffer 會讀不到紋理
			mModel->drawModel(mtinyModel);

			// 5. 解綁 GBuffer
			//glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}

		// =============================================================
		// Phase 2: Compute Pass (Ray Tracing)
		// =============================================================
		{
			mComputeShader->bind();

			// 綁定 GBuffer 紋理供讀取 (Slot 10~13)
			// 注意：這與 default.comp 中的 layout(binding=10) 對應
			mGBuffer->bindForReading(10);

			// 綁定模型紋理 (供次級光線反射時的 hit_world 使用)
			// 這裡假設 max 16 個紋理，綁定到 Slot 20 開始 (避免衝突)
			// 你的 compute shader uniform sampler2D u_textures[16] binding 需要對應修改
			// 或者簡單起見，如果 Compute Shader 邏輯不需要改太大，可以維持原樣
			// 這裡先維持原有的 u_textures 綁定邏輯 (binding=10 in shader code, might need check)
			// [修正]: default.comp 內 u_textures 是 binding 10，這會跟 GBuffer 衝突
			// 請將 default.comp 內的 u_textures 改為 binding 20，並在這裡綁定到 20+
			const auto& textures = mModel->getTextures();
			for (int i = 0; i < std::min((int)textures.size(), 16); ++i) {
				// 假設 Compute Shader 裡 u_textures 改成了 binding = 20
				mComputeShader->bindTexture(textures[i], 20 + i); 

				// 若暫時不改 Compute Shader 的 u_textures，會跟 GBuffer 衝突
				// 建議：將 GBuffer 綁定到 binding 0, 1, 2, 3 (image load/store 是 binding 0，要注意)
				// 最安全的做法：default.comp 裡 GBuffer binding = 10, 11, 12, 13
				// u_textures binding = 14 (array)
			}

			// 設定 Uniforms
			CameraData cameraShaderData = camera.GetShaderData();
			glm::mat4 invViewProj = glm::inverse(projection * view);

			mComputeShader->setUniformFloat2("u_resolution", (float)width, (float)height);
			mComputeShader->setUniformFloat1("u_time", (float)nowTime / 1000.0f);
			mComputeShader->setUniformCamera("camera", cameraShaderData);
			mComputeShader->setUniformBool("cameraUpdated", cameraUpdated);
			mComputeShader->setUniformMat4("invViewProj", invViewProj);
			mComputeShader->setUniformInt("SAMPLES_PER_PIXEL", samples_per_pixel);
			mComputeShader->setUniformInt("MAX_DEPTH", max_depth);
			mComputeShader->setUniformFloat3("backgroundColor", 0.5f, 0.5f, 0.5f);
			mComputeShader->setUniformBool("useOBVH", useOBVH);

			// Ping-Pong Frame Buffer
			mCurrentFrame = 1 - mCurrentFrame;
			GLuint writeTex = mScreenTextures[mCurrentFrame];
			GLuint readTex = mScreenTextures[1 - mCurrentFrame];

			mComputeShader->bindImageTexture(writeTex, 0); // binding 0: output image
			mComputeShader->bindTexture(readTex, 1);       // binding 1: prev frame

			mComputeShader->DispatchCompute(writeTex);
		}

		// =============================================================
		// Phase 3: Post-Processing Pass (Display to Screen/ImGui FBO)
		// =============================================================
		{
			// 取得 Compute Shader 算完的結果
			GLuint finalImage = mScreenTextures[mCurrentFrame];

			// 啟用後處理 Shader
			mShader->bind();

			// 傳入紋理 (Texture Unit 0)
			mShader->bindTexture(finalImage, 0, "screenTexture");

			// 繪製全螢幕四邊形 (寫入 mShader 內部的 FBO)
			mShader->draw(width, height);
		}


		/*uint32_t writeTex = mScreenTextures[mCurrentFrame];
		mComputeShader->DispatchCompute(writeTex);
		mShader->bindTexture(writeTex, 0, "screenTexture");
		mShader->draw(getWindowProperties().width, getWindowProperties().height);*/
	}

	void imguiRender() override
	{
		ImGui::DockSpaceOverViewport(ImGui::GetMainViewport()->ID);
		/*ImGui::DockSpaceOverViewport(ImGui::GetMainViewport()->ID);*/
		//ImGuiDockNodeFlags_PassthruCentralNode
		ImGuiIO& io = ImGui::GetIO();
		ImGui::ShowDemoWindow();


		if (ImGui::Begin("Settings"))
		{
			ImGui::Text("Render Stats:");
			ImGui::Text("FPS: %.1f (%.3f ms)", io.Framerate, 1000.0f / io.Framerate);
			ImGui::Text("Triangles: %d", triangleCount);

			ImGui::Separator();
			ImGui::DragInt("Samples", &samples_per_pixel, 1, 1, 16);
			ImGui::DragInt("Max Bounces", &max_depth, 1, 1, 10);
			ImGui::Checkbox("Use OBVH", &useOBVH);

			ImGui::Separator();
			bool moved = false;
			moved |= ImGui::DragFloat3("Cam Pos", &camera.Position.x, 0.1f);
			if (moved) GuiCameraChanged = true;

			float speed = mCameraController->GetSpeed();
			if (ImGui::DragFloat("Cam Speed", &speed, 0.1f)) mCameraController->SetSpeed(speed);
		}
		ImGui::End();

		/*if (ImGui::Begin("GBuffer Debug"))
		{
			float w = 320, h = 180;
			ImGui::Text("Position");
			ImGui::Image((void*)(intptr_t)mGBuffer->getTexture(graphics::Gbuffer::GBUFFER_POSITION), { w, h }, { 0,1 }, { 1,0 });
			ImGui::Text("Normal");					
			ImGui::Image((void*)(intptr_t)mGBuffer->getTexture(graphics::Gbuffer::GBUFFER_NORMAL), { w, h }, { 0,1 }, { 1,0 });
			ImGui::Text("Albedo");					
			ImGui::Image((void*)(intptr_t)mGBuffer->getTexture(graphics::Gbuffer::GBUFFER_ALBEDO), { w, h }, { 0,1 }, { 1,0 });
		}
		ImGui::End();*/

		/*if (ImGui::Begin("Test2"))
		{
			ImGui::DragInt("samples_per_pixel", &samples_per_pixel, 0.1f);
			ImGui::DragInt("max_depth", &max_depth, 0.1f);
		}
		ImGui::End();

		if (ImGui::Begin("Test3"))
		{
			bool moved = false;
			moved |= ImGui::DragFloat("CamearX", &camera.Position.x, 0.01f);
			moved |= ImGui::DragFloat("CamearY", &camera.Position.y, 0.01f);
			moved |= ImGui::DragFloat("CamearZ", &camera.Position.z, 0.01f);

			if (moved) GuiCameraChanged = true;
		}
		ImGui::End();*/

		if (ImGui::Begin("Metrics/Debugger"))
		{
			
			ImGui::Text("Average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
			ImGui::Text("%d vertices,\n%d indices (%d triangles)", io.MetricsRenderVertices, io.MetricsRenderIndices, triangleCount);
			// ImGui::Text("%d, %d",Engine::Instance().getWindow().getWindowSize().x, Engine::Instance().getWindow().getWindowSize().y);
		
		}
		ImGui::End();


		if (ImGui::Begin("Sence"))
		{
			if (ImGui::IsItemHovered() || ImGui::IsWindowHovered())
			{
				ImGui::SetNextFrameWantCaptureMouse(false);
			}

			ImGui::Image(
					(void*)(intptr_t)mShader->getTexture(),
					// (void*)(uintptr_t)mComputeShader->getTexture(),
					{1280, 720 },
					ImVec2(0, 1),
					ImVec2(1, 0));
		}
		ImGui::End();

		if (ImGui::Begin("Camera Controller"))
		{
			// 你現在可以直接調整控制器的參數
			float speed = mCameraController->GetSpeed();
			if (ImGui::DragFloat("Move Speed", &speed, 0.1f)) {
				mCameraController->SetSpeed(speed);
			}

			ImGui::Separator();
			ImGui::Text("Camera Pos: (%.2f, %.2f, %.2f)", camera.Position.x, camera.Position.y, camera.Position.z);
		}
		ImGui::End();

		ImGui::Begin("My Window");

		if (ImGui::Button("Screenshot")) {
			int num = 0;
			std::string path;
			while (true)
			{
			// "image\\output.png"
				
				path = "image\\output_" + std::to_string(num) + ".png";
				if (!std::filesystem::exists(path)) break;
				num++;
			}
			mShader->exportPNG(path.c_str(), getWindowProperties().width, getWindowProperties().height);
		}

		ImGui::End();
	}

};


static XEngine::App* createApp()
{
	return new Editor();
}

//XEngine::App* createApp();

int main(int argc, char const *argv[])
{
	XEngine::App* app = createApp();
	XEngine::Engine::Instance().run(app);
	
	delete app;
	return 0;
}