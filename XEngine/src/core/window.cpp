#include "core/window.h"
#include "engine.h"
#include "log.h"

#include "SDL3/SDL.h"
#include "glad/glad.h"
#include "external/imgui/imgui.h"
#include "external/imgui/imgui_impl_sdl3.h"


#include "app.h"
#include "input/mouse.h"
#include "input/keyboard.h"



namespace XEngine::core
{
	WindowProperties::WindowProperties()
	{
		title = "RayTracing Project";
		width = 1280;
		height = 720;
		clearColor = glm::vec3(0);
		flags = SDL_WINDOW_OPENGL;
	}


	Window::Window() : mWindow(nullptr), mGLContext(nullptr){}
	Window::~Window()
	{
		if (mWindow)
		{
			shutdown();
		}
	}


	bool Window::create(const WindowProperties& props)
	{
		mWindow = SDL_CreateWindow(props.title.c_str(), props.width, props.height, props.flags);
		if (!mWindow)
		{
			XENGINE_ERROR("Error creating windows: {}", SDL_GetError());
			return false;
		}

#if XENGINE_PLATFORM_MAC
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
#endif
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
		SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);


		SDL_SetWindowMinimumSize(mWindow, 200, 200);

		mGLContext = SDL_GL_CreateContext(mWindow);
		if (mGLContext == nullptr)
		{
			XENGINE_ERROR("Error creating OpenGL context: {}", SDL_GetError());
			return false;
		}

	 	gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress);
		
		SDL_GL_SetSwapInterval(0); // disable vsync

		SDL_WarpMouseInWindow(mWindow,500,500);

		//SDL_SetWindowRelativeMouseMode(mWindow, true);
		mImguiwindow.create(props.imguiProps);
		return true;

	}

	void Window::shutdown()
	{
		SDL_GL_DestroyContext((SDL_GLContextState*)mGLContext);
		SDL_DestroyWindow(mWindow);
		mWindow = nullptr;
		SDL_Quit();
	}

	void Window::pumpEvents()
	{
		// --- 步驟 1: 在所有事件處理之前，重置上一幀的輸入狀態 ---
		// 這確保了 getDeltaX() 等函數在處理新事件前返回 0。
		input::Mouse::update();
		input::Keyboard::update(); // 假設 Keyboard 也有類似的 update 邏輯


		// --- 步驟 2: 處理本幀的所有新事件 ---
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			// 首先讓 ImGui 處理事件，它可能會「消耗」掉事件
			mImguiwindow.handleSDLEvent(event);

			// 檢查 ImGui 是否想要捕獲輸入。如果是，我們自己的遊戲邏輯就不應該響應。
			bool isMouseCapturedByImgui = mImguiwindow.wantCaptureMouse();
			bool isKeyboardCapturedByImgui = mImguiwindow.wantCaptureKeyboard();

			// 處理 QUIT 事件 (總是要處理)
			if (event.type == SDL_EVENT_QUIT) {
				Engine::Instance().quit();
			}

			// 如果鼠標沒有被 ImGui 捕獲，則傳遞給我們的鼠標處理器
			if (!isMouseCapturedByImgui) {
				input::Mouse::ProcessMouseEvent(event);
			}

			// 如果鍵盤沒有被 ImGui 捕獲，則傳遞給我們的鍵盤處理器
			if (!isKeyboardCapturedByImgui) {
				input::Keyboard::update();
			}

			// 可以在這裡處理其他全局事件...
		}
	}
	/*void Window::pumpEvents()
	{
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			mImguiwindow.handleSDLEvent(event);
			//ImGui_ImplSDL3_ProcessEvent(&event); // MAKE ImGui FIRST !!
			switch (event.type)
			{
			case SDL_EVENT_QUIT:
				Engine::Instance().quit();
				break;
			default:
				break;
			}	
		}


		// Update input
		if (!mImguiwindow.wantCaptureMouse())
		{
			input::Mouse::update();
			input::Mouse::ProcessMouseEvent(event);
		}
		if (!mImguiwindow.wantCaptureKeyboard())
		{
			input::Keyboard::update();
		}
		
	}*/

	void Window::beginRender()
	{

	}

	void Window::endRender()
	{
		mImguiwindow.beginRender();
		Engine::Instance().getApp().imguiRender();
		mImguiwindow.endRender();
		SDL_GL_SwapWindow(mWindow);
	}

	glm::ivec2 Window::getWindowSize()
	{
		int w, h;
		SDL_GetWindowSize(mWindow, &w, &h);
		return glm::ivec2(w, h);
	}

	uint64_t Window::getDeltaTime()
	{
		return SDL_GetPerformanceCounter();
	}

	void Window::checkSDLVersion()
	{
		const int compiled = SDL_VERSION;  /* hardcoded number from SDL headers */
		const int linked = SDL_GetVersion();  /* reported by linked SDL library */
		XENGINE_INFO("We compiled against SDL version {}.{}.{}",
			SDL_VERSIONNUM_MAJOR(compiled),
			SDL_VERSIONNUM_MINOR(compiled),
			SDL_VERSIONNUM_MICRO(compiled));

		XENGINE_INFO("But we are linking against SDL version {}.{}.{}.",
			SDL_VERSIONNUM_MAJOR(linked),
			SDL_VERSIONNUM_MINOR(linked),
			SDL_VERSIONNUM_MICRO(linked));
	}
}