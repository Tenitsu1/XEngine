#include "XEngine/engine.h"
#include "XEngine/app.h"

#include "XEngine/log.h"

//#include "XEngine/graphics/vertexbuffer.h"
#include "XEngine/graphics/ComputeShader.h"
#include "XEngine/graphics/shader.h"
#include "XEngine/graphics/framebuffer.h"
#include "XEngine/graphics/gltfLoader.h"
#include "XEngine/graphics/texture.h"

#include "XEngine/input/mouse.h"
#include "XEngine/input/keyboard.h"

#include "external/imgui/imgui.h"
#include "external/glm/glm.hpp"
#include "external/glm/gtc/matrix_transform.hpp"
#include "external/tinygltf/tiny_gltf.h"

using namespace XEngine;

class Editor : public XEngine::App
{

private:
	std::shared_ptr<graphics::Texture> mTexture;
	std::shared_ptr<graphics::ComputeShader> mComputeShader;
	tinygltf::Model mtinyModel;
	std::shared_ptr<graphics::GLTFStaticMesh> mModel;
	GLuint mTriangleSSBO = 0; // 新增 SSBO 的 ID
	int mTriangleCount = 0;   // 三角形數量



	float light = 1000.f;
	float xkeyOffset = 0.f;
	float ykeyOffset = 0.f;
	float zkeyOffset = 0.f;
	float keySpeed = 0.005f;
	float size = 0.5f;
	float samples_per_pixel = 50;

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

		mModel = std::make_shared<graphics::GLTFStaticMesh>(mtinyModel, "models\\just_a_girl\\scene.gltf");
		const auto& triangles = mModel->getTriangles();
		mTriangleCount = (int)triangles.size();
		mComputeShader = std::make_shared<graphics::ComputeShader>("shaders\\default.comp", getWindowProperties().width, getWindowProperties().height);
		mComputeShader->createDebugSSBO(4);
		if (mTriangleCount > 0)
		{
			mComputeShader->createSSBO(mTriangleSSBO, (uint32_t)triangles.size() * sizeof(Triangle), triangles.data(), 3);
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


	}
	void shutdown() override
	{

	}
	void update() override
	{
		auto windowSize = Engine::Instance().getWindow().getWindowSize();
		

		float xNorm = input::Mouse::X() / (float)windowSize.x;
		float yNorm = input::Mouse::Y() / (float)windowSize.y;

		if (input::Keyboard::key(XENGINE_INPUT_KEY_LEFT)) { xkeyOffset -= keySpeed; }
		if (input::Keyboard::key(XENGINE_INPUT_KEY_RIGHT)) { xkeyOffset += keySpeed; }
		if (input::Keyboard::key(XENGINE_INPUT_KEY_UP)) { ykeyOffset += keySpeed; }
		if (input::Keyboard::key(XENGINE_INPUT_KEY_DOWN)) { ykeyOffset -= keySpeed; }

		if (input::Keyboard::keyDown(XENGINE_INPUT_KEY_LEFT)) { xkeyOffset -= keySpeed * 50; }
		if (input::Keyboard::keyDown(XENGINE_INPUT_KEY_RIGHT)) { xkeyOffset += keySpeed * 50; }

		mComputeShader->setUniformFloat2("u_resolution", (float)windowSize.x, (float)windowSize.y);
		mComputeShader->setUniformInt("triangleCount", mTriangleCount);

	}

	void render() override
	{
		auto rc = std::make_unique<graphics::rendercommands::RenderComputeShader>(mComputeShader);

		Engine::Instance().getRenderManager().submit(std::move(rc));

		Engine::Instance().getRenderManager().fulsh();


		static bool hasPrinted = false;
		if (!hasPrinted)
		{
			auto dbgDataOpt = mComputeShader->readDebugData();
			if (dbgDataOpt.has_value())
			{
				const auto& dbgData = dbgDataOpt.value();
				XENGINE_TRACE("--- GPU DEBUG DUMP ---");
				XENGINE_TRACE("Ray Origin: ({:.2f}, {:.2f}, {:.2f})", dbgData.dbg_rayOrigin.x, dbgData.dbg_rayOrigin.y, dbgData.dbg_rayOrigin.z);
				XENGINE_TRACE("Ray Dir:    ({:.2f}, {:.2f}, {:.2f})", dbgData.dbg_rayDir.x, dbgData.dbg_rayDir.y, dbgData.dbg_rayDir.z);
				XENGINE_TRACE("Triangle v0: ({:.2f}, {:.2f}, {:.2f})", dbgData.dbg_v0.x, dbgData.dbg_v0.y, dbgData.dbg_v0.z);
				XENGINE_TRACE("Determinant (a): {:.7f}", dbgData.dbg_det);
				XENGINE_TRACE("u: {:.7f}", dbgData.dbg_u);
				XENGINE_TRACE("v: {:.7f}", dbgData.dbg_v);
				XENGINE_TRACE("t: {:.7f}", dbgData.dbg_t);
				XENGINE_TRACE("----------------------");
				hasPrinted = true;
			}
		}
		mComputeShader->readDebugData();
		
	}

	void imguiRender() override
	{
		ImGui::DockSpaceOverViewport(ImGui::GetMainViewport()->ID);
		/*ImGui::DockSpaceOverViewport(ImGui::GetMainViewport()->ID);*/
		//ImGuiDockNodeFlags_PassthruCentralNode
		
		ImGui::ShowDemoWindow();

		if (ImGui::Begin("Test1"))
		{
			ImGui::DragFloat("PositionX", &xkeyOffset, 0.01f);
			ImGui::DragFloat("PositionY", &ykeyOffset, 0.01f);
			ImGui::DragFloat("PositionZ", &zkeyOffset, 0.01f);
		}
		ImGui::End();
		if (ImGui::Begin("Test2"))
		{
			ImGui::DragFloat("light", &light, 1);
			ImGui::DragFloat("size", &size, 0.01f);
			ImGui::DragFloat("samples_per_pixel", &samples_per_pixel, 0.1f);
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
					(void*)(intptr_t)mComputeShader->getTextureId(),
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