#include "XEngine/engine.h"
#include "XEngine/app.h"
#include "XEngine/log.h"


#include "XEngine/shaders/computeShader.h"
#include "XEngine/shaders/shader.h"

#include "XEngine/graphics/gltfLoader.h"
#include "XEngine/graphics/structs.hpp"
#include "XEngine/graphics/camera.hpp"
#include "XEngine/graphics/cameraController.hpp"

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
	std::shared_ptr<ComputeShader> mComputeShader;
	std::shared_ptr<Shader> mShader;
	tinygltf::Model mtinyModel;
	std::shared_ptr<graphics::GLTFStaticMesh> mModel;
	std::unique_ptr<CameraController> mCameraController;
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


	float light = 1000.f;
	float xkeyOffset = 0.f;
	float ykeyOffset = 0.f;
	float zkeyOffset = 5.f;
	int samples_per_pixel = 1;
	int max_depth = 5;
	Camera camera;
	// Camera camera;
	bool cameraUpdated = false;
	bool GuiCameraChanged = false;
	float lastX = 640, lastY = 450;

	bool useOBVH = false;

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
		// int lightMatIdx = (int)materials.size();
		//Material lightMat;
		//lightMat.baseColorFactor = glm::vec4(1.0f,0.f, 1.f,1.f);
		//lightMat.emissionFactor = glm::vec4(3.0f); // 強度 15 的白光
		//lightMat.type = 1; // Light Type
		//materials.push_back(lightMat);

		//Mesh.materialIndices.push_back(lightMatIdx);
		//Mesh.materialIndices.push_back(lightMatIdx);

		//// Mesh 加入平面光源 正方形
		//float y = 7.9f;
		//glm::vec3 c(-4.0f, y, 0.0f);
		//glm::vec3 offset(2.0f, 0.0f, 2.0f);
		//glm::vec3 v0 = c - offset; // 左下
		//glm::vec3 v1 = c + glm::vec3(offset.x, 0.0f, -offset.z); // 右下
		//glm::vec3 v2 = c + offset; // 右上
		//glm::vec3 v3 = c + glm::vec3(-offset.x, 0.0f, offset.z); // 左上

		//// 加入頂點
		//Mesh.vertices.push_back(glm::vec4(v0, 1.0f));
		//Mesh.vertices.push_back(glm::vec4(v1, 1.0f));
		//Mesh.vertices.push_back(glm::vec4(v2, 1.0f));
		//Mesh.vertices.push_back(glm::vec4(v3, 1.0f));

		//// 加入法線（朝下）
		//glm::vec3 normal(0.0f, -1.0f, 0.0f);
		//for (int i = 0; i < 4; i++)
		//	Mesh.faceNormals.push_back(glm::vec4(normal, 0.0f));

		//// 加入索引（兩個三角形）
		//int baseIdx = (int)Mesh.vertices.size() - 4;
		//Mesh.indices.push_back(glm::ivec4(baseIdx, baseIdx + 1, baseIdx + 2, 0));
		//Mesh.indices.push_back(glm::ivec4(baseIdx, baseIdx + 2, baseIdx + 3, 0));

		//// 加入材質索引
		//Mesh.materialIndices.push_back(lightMatIdx);
		//Mesh.materialIndices.push_back(lightMatIdx);
		

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


		// 計算矩陣
		glm::mat4 view = camera.GetViewMatrix();
		glm::mat4 projection = camera.GetProjectionMatrix((float)windowSize.x, (float)windowSize.y);
		glm::mat4 invViewProj = glm::inverse(projection * view);
		CameraData cameraShaderData = camera.GetShaderData();

		// Compute Shader setup
		mComputeShader->bind();
		mComputeShader->setUniformFloat2("u_resolution", (float)windowSize.x, (float)windowSize.y);
		mComputeShader->setUniformFloat1("u_time", (float)nowTime / 1000.0f);
		mComputeShader->setUniformCamera("camera", cameraShaderData);
		mComputeShader->setUniformBool("cameraUpdated", cameraUpdated);
		mComputeShader->setUniformMat4("invViewProj", invViewProj);
		mComputeShader->setUniformInt("SAMPLES_PER_PIXEL", samples_per_pixel);
		mComputeShader->setUniformInt("MAX_DEPTH", max_depth);
		mComputeShader->setUniformFloat3("backgroundColor", 0.5f, 0.5f, 0.5f);
		mComputeShader->setUniformBool("useOBVH", useOBVH);

		mCurrentFrame = 1 - mCurrentFrame;
		GLuint writeTex = mScreenTextures[mCurrentFrame];
		GLuint readTex  = mScreenTextures[1 - mCurrentFrame];

		// 綁定 image2D 作為輸出
		mComputeShader->bindImageTexture(writeTex, 0);
		// 綁定上一偵作為 sampler2D 輸入
		mComputeShader->bindTexture(readTex, 1); // 1 = prevFrameTexture 的 binding

		const auto& textures = mModel->getTextures();
		if (!textures.empty())
		{
			// 最多綁定 32 個紋理
			int max_textures_to_bind = std::min((int)textures.size(), 32);

			for (int i = 0; i < max_textures_to_bind; ++i)
			{
				// 將紋理綁定到紋理單元 10 + i 
				int textureUnit = 10 + i;
				mComputeShader->bindTexture(textures[i], textureUnit);
			}

			mComputeShader->setUniformInt("u_texture_count", max_textures_to_bind);
		}
	}

	void render() override
	{
		uint32_t writeTex = mScreenTextures[mCurrentFrame];
		mComputeShader->DispatchCompute(writeTex);
		mShader->bindTexture(writeTex, 0, "screenTexture");
		mShader->draw(getWindowProperties().width, getWindowProperties().height);
	}

	void imguiRender() override
	{
		ImGui::DockSpaceOverViewport(ImGui::GetMainViewport()->ID);
		/*ImGui::DockSpaceOverViewport(ImGui::GetMainViewport()->ID);*/
		//ImGuiDockNodeFlags_PassthruCentralNode
		ImGuiIO& io = ImGui::GetIO();
		ImGui::ShowDemoWindow();


		if (ImGui::Begin("Test2"))
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
		ImGui::End();

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