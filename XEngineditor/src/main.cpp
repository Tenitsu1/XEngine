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
#include "XEngine/graphics/scene.hpp" 

#include "XEngine/input/mouse.h"
#include "XEngine/input/keyboard.h"

#include "XEngine/accelerators/bvh.h"

#include "external/imgui/imgui.h"
#include "external/imgui_filebrowser/imfilebrowser.h"
#include "external/glm/glm.hpp"
#include "external/glm/gtc/matrix_transform.hpp"
#include "external/tinygltf/tiny_gltf.h"

#include <string>
#include <filesystem>

using namespace XEngine;


class Editor : public XEngine::App
{

private:

	// Scene mScene;
	std::shared_ptr<Scene> mScene;

	// Shader
	std::shared_ptr<ComputeShader> mComputeShader;
	std::shared_ptr<Shader> mShader;

	// Gbuffer
	std::shared_ptr<graphics::GBuffer> mGBuffer;
	std::shared_ptr<Shader> mGBufferShader;

	// Camera && Controller
	Camera camera;
	std::unique_ptr<CameraController> mCameraController;
	bool cameraUpdated = false;
	bool GuiCameraChanged = false;
	uint64_t nowTime = 0;
	float deltaTime = 0;

	// setting
	bool useOBVH = true;
	int samples_per_pixel = 1;
	int max_depth = 5;
	int mCurrentFrame = 0;
	bool useEnvMap = true;
	float envIntensity = 1.0f;

	GLuint mScreenTexture = 0;

	// File Browser State
	ImGui::FileBrowser mFileDialog;
	std::string defaultScenePath = "models\\cornell_box\\CornellBox_Transmission.gltf";

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

		mScene = std::make_shared<Scene>();

		if (mScene->load(defaultScenePath)) {
			mCurrentFrame = 0;
			cameraUpdated = true;
			mComputeShader->clearTexture(mScreenTexture, width, height);
		}
		mGBuffer = std::make_shared<graphics::GBuffer>();
		if (!mGBuffer->initialize(width, height)) XENGINE_ERROR("Failed to initialize GBuffer!");
		mGBufferShader = std::make_shared<Shader>("shaders\\gbuffer.vert", "shaders\\gbuffer.frag");

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

		mComputeShader = std::make_shared<ComputeShader>("shaders\\test.glsl", width, height);
		mScreenTexture = mComputeShader->createTexture(width, height);

		// Camera setting
		/*camera = Camera(glm::vec3(8.f, 4.0f, 0.2f));*/
		camera = Camera(glm::vec3(0.f, 2.0f, -5.f));
		camera.Yaw = -180.0f;
		camera.Pitch = 0.0f;
		camera.MovementSpeed = 5.0f; 
		camera.ProcessMouseMovement(0, 0);

		mCameraController = std::make_unique<CameraController>(camera);
		mCameraController->SetSpeed(10.0f);

		mFileDialog.SetTitle("Open Scene");
		mFileDialog.SetTypeFilters({ ".gltf", ".glb" });
		mScene->GetBoundsBox();

	}
	void shutdown() override
	{
		mScene->unload();
	}
	void update() override
	{
		auto windowSize = Engine::Instance().getWindow().getWindowSize();

		// Calculate the delta time
		deltaTime = Engine::Instance().getWindow().getDeltaTime(nowTime);

		float timeStep = deltaTime * .1f;


		// Camera Updata
		cameraUpdated = mCameraController->OnUpdate(timeStep) || GuiCameraChanged;
		GuiCameraChanged = false;
	}

	void render() override
	{
		int width = getWindowProperties().width;
		int height = getWindowProperties().height;

		if (!mScene->isLoaded()) return;

		glm::mat4 view = camera.GetViewMatrix();
		glm::mat4 projection = camera.GetProjectionMatrix((float)width, (float)height);

		// =============================================================
		// Phase 1: Geometry Pass (Rasterization -> GBuffer)
		// =============================================================
		{
			mGBuffer->bindForWriting();

			mGBufferShader->bind();

			mGBufferShader->setUniformMat4("view", view);
			mGBufferShader->setUniformMat4("projection", projection);
			mGBufferShader->setUniformMat4("model", glm::mat4(1.0f));

			mGBufferShader->setUniformInt("texture_baseColor", 0);
			mGBufferShader->setUniformInt("texture_metallicRoughness", 1);
			mGBufferShader->setUniformInt("texture_normal", 2);
			mGBufferShader->setUniformInt("texture_emissive", 3);

			mScene->DrawToGBuffer(mGBufferShader);

			mGBufferShader->unbind();
		}

		// =============================================================
		// Phase 2: Compute Pass (Ray Tracing)
		// =============================================================
		{
			mComputeShader->bind();


			mGBuffer->bindForReading(10);

			mScene->BindingToCompute(mComputeShader);

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
			mComputeShader->setUniformBool("useOBVH", useOBVH);

			mComputeShader->setUniformInt("u_envMap", 12);
			//mComputeShader->setUniformBool("useEnvMap", useEnvMap && EnvTextureSSBO != 0);
			mComputeShader->setUniformFloat1("envIntensity", envIntensity);
 

			mComputeShader->DispatchCompute(mScreenTexture);
		}

		// =============================================================
		// Phase 3: Post-Processing Pass (Display to Screen/ImGui FBO)
		// =============================================================
		{
			mShader->bind();
			mShader->bindTexture(mScreenTexture, 0, "screenTexture");
			mShader->draw(width, height);
		}
	}

	void imguiRender() override
	{
		ImGui::DockSpaceOverViewport(ImGui::GetMainViewport()->ID);
		ImGuiIO& io = ImGui::GetIO();

		ImGui::Begin("Properties");

		// -------------------------------------------------------
		// Mesh (檔案載入與資訊)
		// -------------------------------------------------------
		if (ImGui::CollapsingHeader("Mesh", ImGuiTreeNodeFlags_DefaultOpen))
		{
			if (ImGui::Button("Open..."))
			{
				mFileDialog.Open();
			}

			ImGui::SameLine(0, 5.0f);

			std::string fullPath = mScene->getFilePath();
			std::string filename = "None";
			if (!fullPath.empty()) {
				filename = fullPath.substr(fullPath.find_last_of("/\\") + 1);
			}
			ImGui::Text("%s", filename.c_str());

			if (!fullPath.empty()) {
				ImGui::Spacing();
				ImGui::Text("Path:");
				ImGui::Text("%s", fullPath.c_str());
				ImGui::Text("Triangles: %d", mScene->getTriangleCount());
			}
		}

		// -------------------------------------------------------
		// Renderer (參數 & Material)
		// -------------------------------------------------------
		if (ImGui::CollapsingHeader("Renderer Settings", ImGuiTreeNodeFlags_DefaultOpen))
		{
			// 顯示 FPS 與 三角形數量
			ImGui::Text("Render Stats:");
			ImGui::Text("Performance: %.1f FPS (%.3f ms)", ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);
			ImGui::Text("%d vertices,\n%d indices (%d triangles)",
				io.MetricsRenderVertices,
				io.MetricsRenderIndices,
				mScene->getTriangleCount());

			ImGui::Separator();

			bool changed = false;

			// Ray Tracing Params
			GuiCameraChanged |= ImGui::DragInt("Samples", &samples_per_pixel, 1, 1, 100);
			GuiCameraChanged |= ImGui::DragInt("Max Bounces", &max_depth, 1, 1, 20);

			// OBVH
			bool prevOBVH = useOBVH;
			ImGui::Checkbox("Use OBVH", &useOBVH);
			if (prevOBVH != useOBVH) changed = true;

			// Env Map
			ImGui::Separator();
			GuiCameraChanged |= ImGui::Checkbox("Use EnvMap", &useEnvMap);
			if (useEnvMap) {
				changed |= ImGui::DragFloat("Intensity", &envIntensity, 0.1f, 0.0f, 100.0f);
			}
		}

		// -------------------------------------------------------
		// Camera (相機設定)
		// -------------------------------------------------------
		if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Separator();
			ImGui::Text("Transform");
			ImGui::Separator();

			GuiCameraChanged |= ImGui::DragFloat3("Position", &camera.Position.x, 0.1f);
			GuiCameraChanged |= ImGui::DragFloat("Yaw", &camera.Yaw, 1.f, -180.f, 180.f);
			GuiCameraChanged |= ImGui::DragFloat("Pitch", &camera.Pitch,  1.f, -180.f, 180.f);
			GuiCameraChanged |= ImGui::DragFloat("FOV (Zoom)", &camera.Zoom, 1.f, 1.0f, 120.0f);

			float speed = mCameraController->GetSpeed();
			if (ImGui::DragFloat("Move Speed", &speed, 0.1f)) mCameraController->SetSpeed(speed);
		}

		// 截圖按鈕
		ImGui::Separator();
		if (ImGui::Button("Screenshot", ImVec2(-1, 0))) {
			int num = 0;
			std::string path;
			while (true) {
				// "image\\output.png"
				path = "image/output_" + std::to_string(num) + ".png";
				if (!std::filesystem::exists(path)) break;
				num++;
			}
			if (!std::filesystem::exists("image")) std::filesystem::create_directory("image");
			mShader->exportPNG(path.c_str(), getWindowProperties().width, getWindowProperties().height);
		}

		ImGui::End(); // End Properties Window

		// =======================================================
		// Scene Viewport (主渲染畫面)
		// =======================================================
		if (ImGui::Begin("Scene"))
		{
			if (ImGui::IsWindowHovered()) {
				ImGui::SetNextFrameWantCaptureMouse(false);
			}

			ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
			ImGui::Image(
				(void*)(intptr_t)mShader->getTexture(),
				{ 1280, 720 },
				ImVec2(0, 1),
				ImVec2(1, 0));
		}
		ImGui::End();

		// -------------------------------------------------------
		// Debug Views (GBuffer)
		// -------------------------------------------------------
		if (ImGui::Begin("GBuffer Debug"))
		{
			ImGui::Text("Position");
			float w = 320, h = 180;
			ImGui::Image((void*)(intptr_t)mGBuffer->getTexture(graphics::GBuffer::GBUFFER_POSITION), { w, h }, { 0,1 }, { 1,0 });
		}
		ImGui::End();

		if (ImGui::Begin("GBuffer Debug"))
		{
			ImGui::Text("Normal");
			float w = 320, h = 180;
			ImGui::Image((void*)(intptr_t)mGBuffer->getTexture(graphics::GBuffer::GBUFFER_NORMAL), { w, h }, { 0,1 }, { 1,0 });
		}
		ImGui::End();

		if (ImGui::Begin("GBuffer Debug"))
		{
			ImGui::Text("Emission");
			float w = 320, h = 180;
			ImGui::Image((void*)(intptr_t)mGBuffer->getTexture(graphics::GBuffer::GBUFFER_EMISSION), { w, h }, { 0,1 }, { 1,0 });
		}
		ImGui::End();

		// =======================================================
		// File Dialog Logic 
		// =======================================================
		mFileDialog.Display();

		if (mFileDialog.HasSelected())
		{
			mScene->unload();
			std::string selectedPath = mFileDialog.GetSelected().string();

			// 載入模型
			if (mScene->load(selectedPath))
			{
				mCurrentFrame = 0;
				cameraUpdated = true;

				// 清除殘影
				int w = getWindowProperties().width;
				int h = getWindowProperties().height;
				mComputeShader->clearTexture(mScreenTexture, w, h);

				XENGINE_INFO("Scene loaded: {}", selectedPath);
			}

			mFileDialog.ClearSelected();
		}
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