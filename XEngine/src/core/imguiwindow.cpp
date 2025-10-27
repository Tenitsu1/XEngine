#include "core/imguiwindow.h"


#include "engine.h"
#include "SDL3/SDL.h"
#include "external//imgui/imgui.h"
#include "external/imgui/imgui_impl_sdl3.h"
#include "external/imgui/imgui_impl_opengl3.h"

namespace XEngine::core
{
	void ImguiWindow::create(const ImguiWindowProperties& props)
	{
		IMGUI_CHECKVERSION();

		ImGui::CreateContext();

		ImGuiIO& io = ImGui::GetIO();
		io.ConfigWindowsMoveFromTitleBarOnly = props.moveFromTitleBarOnly;
		if (props.isDockingEnable)
		{
			io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		}

		if (props.isViewportEnable)
		{
			io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
		}

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

	bool ImguiWindow::wantCaptureMouse()
	{
		return ImGui::GetIO().WantCaptureMouse;
	}

	bool ImguiWindow::wantCaptureKeyboard()
	{
		return ImGui::GetIO().WantCaptureKeyboard;
	}

	void ImguiWindow::beginRender()
	{ 
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL3_NewFrame();
		//Engine::Instance().getWindow().getSDLWindow();
		ImGui::NewFrame();
	}

	void ImguiWindow::endRender()
	{
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		ImGuiIO& io = ImGui::GetIO();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			auto& window = Engine::Instance().getWindow();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			SDL_GL_MakeCurrent(window.getSDLWindow(), window.getGLContext());
		}
	}
}