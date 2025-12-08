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

	void ImguiWindow::setDarkTheme() {
		ImGuiStyle& style = ImGui::GetStyle();

		// Enable default dark style
		ImGui::StyleColorsDark();

		// Tweaks to match the screenshot better
		style.WindowRounding = 5.3f;
		style.FrameRounding = 2.3f;
		style.ScrollbarRounding = 0;

		style.Colors[ImGuiCol_Text] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
		style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
		style.Colors[ImGuiCol_WindowBg] = ImVec4(0.15f, 0.16f, 0.17f, 1.00f); // Main background (Dark Grey)
		style.Colors[ImGuiCol_ChildBg] = ImVec4(0.15f, 0.16f, 0.17f, 1.00f);
		style.Colors[ImGuiCol_PopupBg] = ImVec4(0.11f, 0.11f, 0.14f, 0.92f);
		style.Colors[ImGuiCol_Border] = ImVec4(0.50f, 0.50f, 0.50f, 0.50f);
		style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		style.Colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.22f, 0.24f, 1.00f); // Input fields
		style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.30f, 0.32f, 0.35f, 1.00f);
		style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.35f, 0.38f, 0.40f, 1.00f);
		style.Colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
		style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.15f, 0.16f, 0.17f, 1.00f);
		style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
		style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
		style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.10f, 0.10f, 0.10f, 0.00f);
		style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
		style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
		style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
		style.Colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
		style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		style.Colors[ImGuiCol_Button] = ImVec4(0.20f, 0.22f, 0.24f, 1.00f); // Buttons
		style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.25f, 0.28f, 0.30f, 1.00f);
		style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.15f, 0.16f, 0.17f, 1.00f);
		style.Colors[ImGuiCol_Header] = ImVec4(0.20f, 0.22f, 0.24f, 1.00f); // Selection
		style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.25f, 0.28f, 0.30f, 1.00f);
		style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.15f, 0.16f, 0.17f, 1.00f);
		style.Colors[ImGuiCol_Tab] = ImVec4(0.15f, 0.16f, 0.17f, 1.00f);
		style.Colors[ImGuiCol_TabHovered] = ImVec4(0.35f, 0.38f, 0.40f, 1.00f);
		style.Colors[ImGuiCol_TabActive] = ImVec4(0.28f, 0.30f, 0.32f, 1.00f);
		style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.15f, 0.16f, 0.17f, 1.00f);
		style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.20f, 0.22f, 0.24f, 1.00f);
	}
}