#pragma once

#include "core/imguiwindow.h"
#include <string>
#include <memory>

#include "external/glm/glm.hpp"

namespace XEngine::graphics
{
	class Framebuffer;
}


struct SDL_Window; 
//using SDL_GLContext = void*;
typedef struct SDL_GLContextState* SDL_GLContext;

namespace XEngine::core
{
	//using SDL_GLContext = void*;

	struct WindowProperties
	{
		std::string title;
		int width, height;
		int flags;
		glm::vec3 clearColor;
		ImguiWindowProperties imguiProps;
		WindowProperties();
	};

	class Window
	{
	public:
		Window();
		~Window();
		
		bool create(const WindowProperties& props);
		void shutdown();

		static bool initialize();

		void pumpEvents();

		static void checkSDLVersion();
		glm::ivec2 getWindowSize();
		uint64_t getDeltaTime();

		inline SDL_Window* getSDLWindow() { return mWindow; }
		inline SDL_GLContext getGLContext() { return mGLContext; }

		void beginRender();
		void endRender();

	private:
		SDL_Window* mWindow;
		SDL_GLContext mGLContext;
		ImguiWindow mImguiwindow;
	};
}