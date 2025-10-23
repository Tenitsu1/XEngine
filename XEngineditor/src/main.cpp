#include "XEngine/engine.h"
#include "XEngine/app.h"

#include "XEngine/log.h"

#include "XEngine/graphics/mesh.h"
#include "XEngine/graphics/shader.h"

#include "XEngine/input/mouse.h"
#include "XEngine/input/keyboard.h"

#include "external/imgui/imgui.h"

using namespace XEngine;

class Editor : public XEngine::App
{
private:
	std::shared_ptr<graphics::Mesh> mMesh;
	std::shared_ptr<graphics::Shader> mShader;

	float light = 0.f;
	float xkeyOffset = 0.f;
	float ykeyOffset = 0.f;
	float zkeyOffset = 0.f;
	float keySpeed = 0.005f;
	float size = 1.0f;

public:
	void initialize() override
	{
		// Test Mesh
		/*float vertice[]
		{
			 1.0f,  1.0f, 0.f,
			 1.0f, -1.0f, 0.f,
			-1.0f, -1.0f, 0.f,
			-1.0f,  1.0f, 0.f,
		};*/
		float vertice[] =
		{ 
				-1, -1,
				 1, -1,
				 1,  1,
				-1,  1
		};
		uint32_t elements[]
		{
				0, 3, 1,
				1, 3, 2
		};
		mMesh = std::make_shared<graphics::Mesh>(&vertice[0], 4, 2, &elements[0], 6);
		mShader = std::make_shared<graphics::Shader>("shaders\\default.vert", "shaders\\default.frag");
		//shader->setUniformFloat3("color", 255, 0, 0);
	}
	void shutdown() override
	{
	}
	void update() override
	{
		int windowWidth = 0;
		int windowHeight = 0;
		Engine::Instance().getWindow().getWindowSize(windowWidth, windowHeight);

		float xNorm = input::Mouse::X() / (float)windowWidth;
		float yNorm = input::Mouse::Y() / (float)windowWidth;

		if (input::Keyboard::key(XENGINE_INPUT_KEY_LEFT)) { xkeyOffset -= keySpeed; }
		if (input::Keyboard::key(XENGINE_INPUT_KEY_RIGHT)) { xkeyOffset += keySpeed; }
		if (input::Keyboard::key(XENGINE_INPUT_KEY_UP)) { ykeyOffset += keySpeed; }
		if (input::Keyboard::key(XENGINE_INPUT_KEY_DOWN)) { ykeyOffset -= keySpeed; }

		if (input::Keyboard::keyDown(XENGINE_INPUT_KEY_LEFT)) { xkeyOffset -= keySpeed * 50; }
		if (input::Keyboard::keyDown(XENGINE_INPUT_KEY_RIGHT)) { xkeyOffset += keySpeed * 50; }
		mShader->setUniformFloat2("u_resolution", 800.0f, 600.0f);
		mShader->setUniformInt("sphereCount", 4);
		mShader->setUniformFloat4("spheres[0]", 0.0f, -100.5f, -1.0f, 100.0f); // 地面
		mShader->setUniformFloat4("spheres[1]", 0.2f, 0.8f, 0.2f, 0.0f); // 地面顏色
		mShader->setUniformFloat4("spheres[2]", 0.0f+xkeyOffset, 0.0f+ykeyOffset, -1.2f+ zkeyOffset, size); // 大球
		mShader->setUniformFloat4("spheres[3]", light, light, light, 0.0f); // 大球顏色
		mShader->setUniformFloat4("spheres[4]", -0.7f, -0.3f, -1.0f, 0.2f); // 左球
		mShader->setUniformFloat4("spheres[5]", 0.0f, 1.0f, 0.0f, 0.0f); // 左球顏色
		mShader->setUniformFloat4("spheres[6]", -0.3f, -0.3f, -1.0f, 0.2f); // 右球
		mShader->setUniformFloat4("spheres[7]", 1.0f, 0.0f, 0.0f, 0.0f); // 右球顏色
		mShader->setUniformFloat3("cameraPos", 0.0f, 0.0f, 0.0f);
		mShader->setUniformFloat3("cameraTarget", 0.0f, 0.0f, -1.0f);
		mShader->setUniformFloat1("cameraFov", 1.5f);
		mShader->setUniformFloat1("samples_per_pixel", 100.0f);
		mShader->setUniformFloat3("backgroundColor", 0.6f, 0.8f, 1.0f);
		mShader->setUniformInt("sphereCount", 4);
		mShader->setUniformInt("max_depth", 10);
		//mShader->setUniformFloat3("offset", xNorm + xkeyOffset, yNorm + ykeyOffset, yNorm + ykeyOffset);

	}
	void render() override
	{
		auto rc = std::make_unique<graphics::rendercommands::RenderMesh>(mMesh, mShader);
		Engine::Instance().getRenderManager().submit(std::move(rc));
		Engine::Instance().getRenderManager().fulsh();
	}

	void imguiRender() override
	{
		ImGui::ShowDemoWindow();
		if (ImGui::Begin("Test1"))
		{
			ImGui::DragFloat("PositionX", &xkeyOffset, 0.01f);
			ImGui::DragFloat("PositionY", &ykeyOffset, 0.01f);
			ImGui::DragFloat("PositionZ", &zkeyOffset, 0.01f);
			ImGui::DragFloat("light", &light, 1);
			ImGui::DragFloat("size", &size, 0.1f);
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