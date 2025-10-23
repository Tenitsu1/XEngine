#include "core/imguiwindow.h"


#include "engine.h"
#include "external//imgui/imgui.h"
#include "external/imgui/imgui_impl_sdl3.h"
#include "external/imgui/imgui_impl_opengl3.h"

namespace XEngine::core
{
	void ImguiWindow::create()
	{
		IMGUI_CHECKVERSION();

		ImGui::CreateContext();

		auto& window = Engine::Instance().getWindow();
		ImGui_ImplSDL3_InitForOpenGL(window.getSDLWindow(), window.getGLContext());
		ImGui::StyleColorsDark();
		ImGui_ImplOpenGL3_Init("#version 460");
	}

	void ImguiWindow::shutdown()
	{
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplSDL3_Shutdown();
		ImGui::DestroyContext();
	}

	void ImguiWindow::handleSDLEvent(SDL_Event& e)
	{
		ImGui_ImplSDL3_ProcessEvent(&e);
	}

	void ImguiWindow::beginRender()
	{ 
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL3_NewFrame();
		// Engine::Instance().getWindow().getSDLWindow()
		ImGui::NewFrame();
	}

	void ImguiWindow::endRender()
	{
		
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	}
}