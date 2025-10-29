#include "core/window.h"
#include "engine.h"
#include "log.h"

#include "SDL3/SDL.h"
#include "glad/glad.h"
#include "external/imgui/imgui.h"
#include "external/imgui/imgui_impl_sdl3.h"


#include "app.h"
#include "graphics/framebuffer.h"

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

		mFramebuffer = std::make_shared<graphics::Framebuffer>(props.width, props.height);
		glm::vec4 clearColor{ props.clearColor.r, props.clearColor.g , props.clearColor.b , 1.0f };
		mFramebuffer->setClearColor(clearColor);

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
		}
		if (!mImguiwindow.wantCaptureKeyboard())
		{
			input::Keyboard::update();
		}
		
	}

	void Window::beginRender()
	{
		Engine::Instance().getRenderManager().clear();
		auto cmd = std::make_unique<graphics::rendercommands::PushFramebuffer>(mFramebuffer);
		Engine::Instance().getRenderManager().submit(std::move(cmd));
	}

	void Window::endRender()
	{
		
		auto cmd = std::make_unique<graphics::rendercommands::PopFramebuffer>();
		Engine::Instance().getRenderManager().submit(std::move(cmd));
		Engine::Instance().getRenderManager().fulsh();
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