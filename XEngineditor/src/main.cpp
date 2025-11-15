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

using namespace XEngine;

class Editor : public XEngine::App
{

private:
	std::shared_ptr<ComputeShader> mComputeShader;
	std::shared_ptr<Shader> mShader;
	tinygltf::Model mtinyModel;
	std::shared_ptr<graphics::GLTFStaticMesh> mModel;
	// GLuint mTriangleSSBO = 0; // 新增 SSBO 的 ID
	GLuint mVerticesSSBO = 0; // 新增頂點 SSBO 的 ID
	GLuint mIndicesSSBO = 0;  // 新增索引 SSBO 的 ID
	GLuint mNormalsSSBO = 0;  // 新增法線 SSBO 的 ID
	GLuint mOBVHSSBO = 0;     // OBVH SSBO ID
	GLuint mTexCoordsSSBO = 0;
	int mTriangleCount = 0;   // 三角形數量

	uint64_t nowTime = Engine::Instance().getWindow().getDeltaTime();
	uint64_t laseTime = 0;
	float deltaTime = 0;

	float light = 1000.f;
	float xkeyOffset = 0.f;
	float ykeyOffset = 0.f;
	float zkeyOffset = 5.f;
	float keySpeed = 0.005f;
	float size = 0.5f;
	int samples_per_pixel = 1;
	Camera camera;
	float lastX = 640, lastY = 450;
	bool firstMouse = true;

public:

	core::WindowProperties getWindowProperties()
	{
		core::WindowProperties props;
		props.title = "RayTracing Projecrelihifejpiot";
		props.width = 1600;
		props.height = 900;
		props.imguiProps.isViewportEnable = true;
		props.imguiProps.isDockingEnable = true;
		return props;
	}

	void initialize() override
	{

		mModel = std::make_shared<graphics::GLTFStaticMesh>(mtinyModel, "models\\TexCube2.gltf");
		auto& Mesh = mModel->getMesh();
		mTriangleCount = (int)Mesh.indices.size();
		mShader = std::make_shared<Shader>("shaders\\default.vert", "shaders\\default.frag");
		mComputeShader = std::make_shared<ComputeShader>("shaders\\default.comp", getWindowProperties().width, getWindowProperties().height);
		mComputeShader->createDebugSSBO(7);
		auto obvhNodes = OBVH::buildOBVH(Mesh);
		if (mTriangleCount > 0)
		{
			mComputeShader->createSSBO(mOBVHSSBO, (uint32_t)obvhNodes.size() * sizeof(OBVH::OBVHNode), obvhNodes.data(), 1);
			mComputeShader->createSSBO(mVerticesSSBO, (uint32_t)Mesh.vertices.size() * sizeof(glm::vec4), Mesh.vertices.data(), 2);
			mComputeShader->createSSBO(mIndicesSSBO, (uint32_t)Mesh.indices.size() * sizeof(glm::ivec4), Mesh.indices.data(), 3);
			mComputeShader->createSSBO(mNormalsSSBO, (uint32_t)Mesh.normals.size() * sizeof(glm::vec4), Mesh.normals.data(), 4);
			XENGINE_TRACE("Vertices SSBO created, size: {}, count: {}",
				(uint32_t)Mesh.vertices.size() * sizeof(glm::vec2), Mesh.vertices.size());
			XENGINE_TRACE("Indices SSBO created, size: {}, count: {}",
				(uint32_t)Mesh.indices.size() * sizeof(glm::vec2), Mesh.indices.size());
			XENGINE_TRACE("Normals SSBO created, size: {}, count: {}",
				(uint32_t)Mesh.normals.size() * sizeof(glm::vec2), Mesh.normals.size());
			if (!Mesh.texCoords.empty()) {
				mComputeShader->createSSBO(mTexCoordsSSBO, (uint32_t)Mesh.texCoords.size() * sizeof(glm::vec2), Mesh.texCoords.data(), 5);
				XENGINE_TRACE("TexCoords SSBO created, size: {}, count: {}",
					(uint32_t)Mesh.texCoords.size() * sizeof(glm::vec2), Mesh.texCoords.size());
			}
			else {
				XENGINE_WARN("Model has no texture coordinates!");
			}

		}


		// --- 新增：印出包圍盒日誌 ---
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
		camera.position = glm::vec3(-5.0, 0.0, 0.0);
		camera.lookat = glm::vec3(0.0f, 0.0f, -1.0f);
		camera.up = glm::vec3(0.0f, 1.0f, 0.0f);
		camera.fov = 45.0f;

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
		

		// Camera Updata
		if (input::Keyboard::key(XENGINE_INPUT_KEY_DOWN)) { camera.position -= keySpeed * camera.lookat; }
		if (input::Keyboard::key(XENGINE_INPUT_KEY_UP)) { camera.position += keySpeed * camera.lookat; }
		if (input::Keyboard::key(XENGINE_INPUT_KEY_LEFT)) { camera.position -= glm::normalize(glm::cross(camera.lookat, camera.up)) * keySpeed; }
		if (input::Keyboard::key(XENGINE_INPUT_KEY_RIGHT)) { camera.position += glm::normalize(glm::cross(camera.lookat, camera.up)) * keySpeed; }
		if (input::Keyboard::key(XENGINE_INPUT_KEY_SPACE)) { ykeyOffset += keySpeed; }
		if (input::Keyboard::key(XENGINE_INPUT_KEY_LSHIFT)) { ykeyOffset -= keySpeed; }

		//if (input::Keyboard::keyDown(XENGINE_INPUT_KEY_LEFT)) { xkeyOffset -= keySpeed ; }
		//if (input::Keyboard::keyDown(XENGINE_INPUT_KEY_RIGHT)) { xkeyOffset += keySpeed; }
		//if (input::Keyboard::keyDown(XENGINE_INPUT_KEY_UP)) { zkeyOffset -= keySpeed * deltaTime; }
		//if (input::Keyboard::keyDown(XENGINE_INPUT_KEY_DOWN)) { zkeyOffset += keySpeed * deltaTime; }
		//if (input::Keyboard::keyDown(XENGINE_INPUT_KEY_SPACE)) { ykeyOffset += keySpeed * deltaTime; }
		//if (input::Keyboard::keyDown(XENGINE_INPUT_KEY_LSHIFT)) { ykeyOffset -= keySpeed * deltaTime; }

		// Mouse and keyborad input 
		//float xNorm = input::Mouse::X() / (float)windowSize.x;
		//float yNorm = input::Mouse::Y() / (float)windowSize.y;
		float xNorm = input::Mouse::X();
		float yNorm = input::Mouse::Y();
		if (firstMouse)
		{
			camera.pitch = 0.0f;
			lastX = xNorm;
			lastY = yNorm;
			firstMouse = false;
		}

		xkeyOffset = xNorm - lastX;
		ykeyOffset = lastY - yNorm;
		lastX = xNorm;
		lastY = yNorm;
		float sensitivity = 0.05f;
		xkeyOffset *= sensitivity;
		ykeyOffset *= sensitivity;

		camera.yaw += xkeyOffset;
		camera.pitch += ykeyOffset;

		if (camera.pitch > 89.0f)
			camera.pitch = 89.0f;
		if (camera.pitch < -89.0f)
			camera.pitch = -89.0f;

		glm::vec3 front;
		front.x = cos(glm::radians(camera.pitch)) * cos(glm::radians(camera.yaw));
		front.y = sin(glm::radians(camera.pitch));
		front.z = cos(glm::radians(camera.pitch)) * sin(glm::radians(camera.yaw));
		camera.lookat = glm::normalize(front);

		camera.direction.x = cos(glm::radians(camera.pitch)) * cos(glm::radians(camera.yaw)); 
		camera.direction.y = sin(glm::radians(camera.pitch));
		camera.direction.z = cos(glm::radians(camera.pitch)) * sin(glm::radians(camera.yaw));

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
		mComputeShader->setUniformInt("max_depth", 5);
		mComputeShader->setUniformFloat3("backgroundColor", 0.5f, 0.5f, 0.5f);

		const auto& textures = mModel->getTextures();
		if (!textures.empty())
		{
			// 3. 告訴 shader sampler 使用紋理單元 6
			int textureUnit = 6;
			mComputeShader->setUniformInt("u_texture", textureUnit);

			// 4. 將模型的第一個紋理綁定到紋理單元 6
			//    (這裡我們調用假設的 bindTexture 方法)
			GLuint textureID_to_bind = textures[0]; // 假設我們總是使用第一個紋理
			mComputeShader->bindTexture(textureID_to_bind, textureUnit);
		}
		else
		{
			// 可選：如果沒有紋理，可以綁定一個空的或白色的紋理，避免 GPU 出錯
			// mComputeShader->bindTexture(0, 6); // 綁定 0 等於解綁
		}
		
	}

	void render() override
	{
		mComputeShader->DispatchCompute();
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
			ImGui::DragFloat("light", &light, 1);
			ImGui::DragFloat("size", &size, 0.01f);
			ImGui::DragInt("samples_per_pixel", &samples_per_pixel, 0.1f);
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
			ImGui::Text("%d vertices,\n%d indices (%d triangles)", io.MetricsRenderVertices, io.MetricsRenderIndices, mTriangleCount);
		}
		ImGui::End();


		if (ImGui::Begin("Sence"))
		{
			if (ImGui::IsItemHovered() || ImGui::IsWindowHovered())
			{
				ImGui::SetNextFrameWantCaptureMouse(false);
			}
			auto& window = Engine::Instance().getWindow();

			ImGui::Image(
					(void*)(intptr_t)mComputeShader->getTexture(),
					{1280, 720 },
					ImVec2(0, 1),
					ImVec2(1, 0));
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