#include "core/window.h"
#include "engine.h"
#include "log.h"

#include "SDL3/SDL.h"
#include "glad/glad.h"
#include "external/imgui/imgui.h"

#include "app.h"

#include "input/mouse.h"
#include "input/keyboard.h"



namespace XEngine::core
{
	Window::Window() : mWindow(nullptr), mGLContext(nullptr){}
	Window::~Window()
	{
		if (mWindow)
		{
			shutdown();
		}
	}


	bool Window::create()
	{
		mWindow = SDL_CreateWindow("RayTracing", 1200, 768 ,SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
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


		SDL_SetWindowMinimumSize(mWindow, 200, 200);

		mGLContext = SDL_GL_CreateContext(mWindow);
		if (mGLContext == nullptr)
		{
			XENGINE_ERROR("Error creating OpenGL context: {}", SDL_GetError());
			return false;
		}

	 	gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress);
		
		SDL_GL_SetSwapInterval(0); // disable vsync

		mImguiwindow.create();
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
		SDL_Event e;
		while (SDL_PollEvent(&e))
		{
			mImguiwindow.handleSDLEvent(e);
			//ImGui_ImplSDL3_ProcessEvent(&e); // MAKE ImGui FIRST !!
			switch (e.type)
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
		}
		if (!mImguiwindow.wantCaptureKeyboard())
		{
			input::Keyboard::update();
		}
		
	}

	void Window::beginRender()
	{
		Engine::Instance().getRenderManager().clear();
	}

	void Window::endRender()
	{
		
		mImguiwindow.beginRender();
		Engine::Instance().getApp().imguiRender();
		mImguiwindow.endRender();
		SDL_GL_SwapWindow(mWindow);
	}

	void Window::getWindowSize(int& w, int& h)
	{
		SDL_GetWindowSize(mWindow, &w, &h);
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