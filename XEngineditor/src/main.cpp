#include "XEngine/engine.h"
#include "XEngine/app.h"
#include "XEngine/log.h"


#include "XEngine/shaders/computeShader.h"
#include "XEngine/shaders/shader.h"

#include "XEngine/graphics/gltfLoader.h"
#include "XEngine/graphics/structs.hpp"

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
	GLuint mVerticesSSBO = 0; // vertex  SSBO-ID
	GLuint mIndicesSSBO = 0;  // indices SSBO-ID
	GLuint mNormalsSSBO = 0;  // normal  SSBO-ID
	GLuint mOBVHSSBO = 0;     // OBVH    SSBO-ID
	GLuint mFaceNormalsSSBO = 0;
	GLuint mTexCoordsSSBO = 0;
	GLuint mMaterialIndicesSSBO = 0;
	GLuint mMatToTexMapSSBO = 0;
	std::vector<int> matToTexMap;

	uint64_t nowTime = Engine::Instance().getWindow().getDeltaTime();
	uint64_t laseTime = 0;
	float deltaTime = 0;
	int triangleCount = 0; 

	float light = 1000.f;
	float xkeyOffset = 0.f;
	float ykeyOffset = 0.f;
	float zkeyOffset = 5.f;
	float keySpeed = 0.005f;
	float size = 0.5f;
	int samples_per_pixel = 1;
	int max_depth = 5;
	Camera camera;
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

		mModel = std::make_shared<graphics::GLTFStaticMesh>(mtinyModel, "models\\boxWithDog\\scene.gltf");
		auto& Mesh = mModel->getMesh();
		// Mesh 加入平面光源 正方形
		float y = 7.9f;
		glm::vec3 c(-4.0f, y, 0.0f);
		glm::vec3 offset(2.0f, 0.0f, 2.0f);
		glm::vec3 v0 = c - offset; // 左下
		glm::vec3 v1 = c + glm::vec3(offset.x, 0.0f, -offset.z); // 右下
		glm::vec3 v2 = c + offset; // 右上
		glm::vec3 v3 = c + glm::vec3(-offset.x, 0.0f, offset.z); // 左上

		// 加入頂點
		Mesh.vertices.push_back(glm::vec4(v0, 1.0f));
		Mesh.vertices.push_back(glm::vec4(v1, 1.0f));
		Mesh.vertices.push_back(glm::vec4(v2, 1.0f));
		Mesh.vertices.push_back(glm::vec4(v3, 1.0f));

		// 加入法線（朝下）
		glm::vec3 normal(0.0f, -1.0f, 0.0f);
		for (int i = 0; i < 4; i++)
			Mesh.faceNormals.push_back(glm::vec4(normal, 0.0f));

		// 加入索引（兩個三角形）
		int baseIdx = (int)Mesh.vertices.size() - 4;
		Mesh.indices.push_back(glm::ivec4(baseIdx, baseIdx + 1, baseIdx + 2, 0));
		Mesh.indices.push_back(glm::ivec4(baseIdx, baseIdx + 2, baseIdx + 3, 0));

		// 加入材質索引
		int lightMatIdx = 0; // 或你已知的 LIGHT 材質 index
		Mesh.materialIndices.push_back(lightMatIdx);
		Mesh.materialIndices.push_back(lightMatIdx);
		

		triangleCount = (int)Mesh.indices.size();
		mShader = std::make_shared<Shader>("shaders\\default.vert", "shaders\\default.frag");
		mShader->createTexture(getWindowProperties().width, getWindowProperties().height);
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
		mComputeShader = std::make_shared<ComputeShader>("shaders\\default.comp", getWindowProperties().width, getWindowProperties().height);
		mShader->bindTexture(mComputeShader->getTexture(), 0, "screenTexture");
		auto obvhNodes = OBVH::buildOBVH(Mesh);

		if (!mtinyModel.materials.empty())
		{
			matToTexMap.resize(mtinyModel.materials.size());
			for (size_t i = 0; i < mtinyModel.materials.size(); ++i)
			{
				const auto& mat = mtinyModel.materials[i];
				int texture_index = mat.pbrMetallicRoughness.baseColorTexture.index;
				matToTexMap[i] = texture_index; 
			}
		}


		if (triangleCount > 0)
		{
			mComputeShader->createSSBO(mOBVHSSBO,         (uint32_t)obvhNodes.size()         * sizeof(OBVH::OBVHNode), obvhNodes.data(),         1);
			mComputeShader->createSSBO(mVerticesSSBO,     (uint32_t)Mesh.vertices.size()     * sizeof(glm::vec4),      Mesh.vertices.data(),     2);	
			mComputeShader->createSSBO(mIndicesSSBO,      (uint32_t)Mesh.indices.size()      * sizeof(glm::ivec4),     Mesh.indices.data(),      3);
			mComputeShader->createSSBO(mFaceNormalsSSBO,  (uint32_t)Mesh.faceNormals.size()  * sizeof(glm::vec4),      Mesh.faceNormals.data(),  4);
			mComputeShader->createSSBO(mNormalsSSBO,      (uint32_t)Mesh.normals.size()      * sizeof(glm::vec4),      Mesh.normals.data(),      9);

			if (!Mesh.texCoords.empty()) 
			{
				mComputeShader->createSSBO(mTexCoordsSSBO, (uint32_t)Mesh.texCoords.size() * sizeof(glm::vec2), Mesh.texCoords.data(), 5);
			}
			else 
			{
				XENGINE_WARN("Model has no texture coordinates!");
			}

			if (!Mesh.materialIndices.empty()) {
				mComputeShader->createSSBO(mMaterialIndicesSSBO,
					(uint32_t)Mesh.materialIndices.size() * sizeof(int),
					Mesh.materialIndices.data(), 7);
			}
			if (!matToTexMap.empty()) {
				mComputeShader->createSSBO(mMatToTexMapSSBO,
					(uint32_t)matToTexMap.size() * sizeof(int),
					matToTexMap.data(), 8);
			}
			
			/* {
				XENGINE_TRACE("Vertices SSBO created, size: {}, count: {}",
					(uint32_t)Mesh.vertices.size() * sizeof(glm::vec4), Mesh.vertices.size());
				XENGINE_TRACE("Indices SSBO created, size: {}, count: {}",
					(uint32_t)Mesh.indices.size() * sizeof(glm::ivec4), Mesh.indices.size());
				XENGINE_TRACE("Normals SSBO created, size: {}, count: {}",
					(uint32_t)Mesh.normals.size() * sizeof(glm::vec4), Mesh.normals.size());
				XENGINE_TRACE("TexCoords SSBO created, size: {}, count: {}",
					(uint32_t)Mesh.texCoords.size() * sizeof(glm::vec2), Mesh.texCoords.size());
				XENGINE_TRACE("MaterialIndices SSBO created, size: {}, count: {}",
					(uint32_t)Mesh.materialIndices.size() * sizeof(int), Mesh.materialIndices.size());
				XENGINE_TRACE("MatToTexMap SSBO created, size: {}, count: {}",
					(uint32_t)matToTexMap.size() * sizeof(int), matToTexMap.size());
			} */
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


		// Camera Setup
		// camera.position = glm::vec3(-60.f, 90.0f, 80.f);
		// camera.lookat = glm::vec3(0.0f, 0.0f, -1.0f);
		camera.position = glm::vec3(8.f, 4.0f, 0.2f);
		camera.lookat = glm::vec3(-1.0f, 0.0f, 0.0f);
		camera.up = glm::vec3(0.0f, 1.0f, 0.0f);
		camera.fov = 45.0f;

		camera.yaw = -90.0f;  // 或 270.0f
		camera.pitch = 0.0f;

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

		float deltaTimeMax = deltaTime * 100000;
		

		// Camera Updata
		if (input::Keyboard::key(XENGINE_INPUT_KEY_DOWN)) { camera.position -= keySpeed * camera.lookat * deltaTimeMax; }
		if (input::Keyboard::key(XENGINE_INPUT_KEY_UP)) { camera.position += keySpeed * camera.lookat * deltaTimeMax; }
		if (input::Keyboard::key(XENGINE_INPUT_KEY_LEFT)) { camera.position -= glm::normalize(glm::cross(camera.lookat, camera.up)) * keySpeed * deltaTimeMax; }
		if (input::Keyboard::key(XENGINE_INPUT_KEY_RIGHT)) { camera.position += glm::normalize(glm::cross(camera.lookat, camera.up)) * keySpeed * deltaTimeMax; }
		if (input::Keyboard::key(XENGINE_INPUT_KEY_SPACE)) { ykeyOffset += keySpeed * deltaTimeMax; }
		if (input::Keyboard::key(XENGINE_INPUT_KEY_LSHIFT)) { ykeyOffset -= keySpeed * deltaTimeMax; }

		/*float xkeyOffset = input::Mouse::dX();
		float ykeyOffset = input::Mouse::dY();
		float sensitivity = 0.1f;
		xkeyOffset *= sensitivity;
		ykeyOffset *= sensitivity;
		camera.yaw += xkeyOffset;
		camera.pitch -= ykeyOffset;

		if (camera.pitch > 89.0f)
			camera.pitch = 89.0f;
		if (camera.pitch < -89.0f)
			camera.pitch = -89.0f;

		glm::vec3 front;
		front.x = cos(glm::radians(camera.pitch)) * cos(glm::radians(camera.yaw));
		front.y = sin(glm::radians(camera.pitch));
		front.z = cos(glm::radians(camera.pitch)) * sin(glm::radians(camera.yaw));
		camera.lookat = glm::normalize(front);
		camera.right = glm::normalize(glm::cross(camera.lookat, camera.up)); 
		camera.cameraUp = glm::normalize(glm::cross(camera.right, camera.lookat));
		camera.direction = camera.lookat;


		camera.direction.x = cos(glm::radians(camera.pitch)) * cos(glm::radians(camera.yaw)); 
		camera.direction.y = sin(glm::radians(camera.pitch));
		camera.direction.z = cos(glm::radians(camera.pitch)) * sin(glm::radians(camera.yaw));*/

		float fov = glm::radians(camera.fov);
		float aspect = (float)windowSize.x / (float)windowSize.y;
		glm::mat4 view = glm::lookAt(camera.position, camera.position+camera.lookat, camera.up);
		// 近裁剪面（zNear = 0.1f）和遠裁剪面（zFar = 100.0f）
		glm::mat4 projection = glm::perspective(fov, aspect, 0.1f, 100.0f);
		glm::mat4 invViewProj = glm::inverse(projection * view);


		// Compute Shader setup
		mComputeShader->bind();
		mComputeShader->setUniformFloat2("u_resolution", (float)windowSize.x, (float)windowSize.y);
		mComputeShader->setUniformCamera("camera", camera);
		mComputeShader->setUniformMat4("invViewProj", invViewProj);
		mComputeShader->setUniformInt("SAMPLES_PER_PIXEL", samples_per_pixel);
		mComputeShader->setUniformInt("MAX_DEPTH", max_depth);
		mComputeShader->setUniformFloat3("backgroundColor", 0.5f, 0.5f, 0.5f);
		mComputeShader->setUniformBool("useOBVH", useOBVH);

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
		mComputeShader->DispatchCompute();
		mShader->draw(getWindowProperties().width, getWindowProperties().height);
	}

	void imguiRender() override
	{
		ImGui::DockSpaceOverViewport(ImGui::GetMainViewport()->ID);
		/*ImGui::DockSpaceOverViewport(ImGui::GetMainViewport()->ID);*/
		//ImGuiDockNodeFlags_PassthruCentralNode
		ImGuiIO& io = ImGui::GetIO();
		ImGui::ShowDemoWindow();

		if (ImGui::Begin("Test1"))
		{
			ImGui::DragFloat("LookAtX", &camera.lookat.x, 0.01f);
			ImGui::DragFloat("LookAtY", &camera.lookat.y, 0.01f);
			ImGui::DragFloat("LookAtZ", &camera.lookat.z, 0.01f);
		}
		ImGui::End();

		if (ImGui::Begin("Test2"))
		{
			ImGui::DragInt("samples_per_pixel", &samples_per_pixel, 0.1f);
			ImGui::DragInt("max_depth", &max_depth, 0.1f);
		}
		ImGui::End();

		if (ImGui::Begin("Test3"))
		{
			ImGui::DragFloat("CamearX", &camera.position.x, 0.01f);
			ImGui::DragFloat("CamearY", &camera.position.y, 0.01f);
			ImGui::DragFloat("CamearZ", &camera.position.z, 0.01f);
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

		ImGui::Begin("My Window");

		if (ImGui::Button("Click Me")) {
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