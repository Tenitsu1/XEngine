#pragma once

#include "core/imguiwindow.h"

struct SDL_Window; 
//using SDL_GLContext = void*;
namespace XEngine::core
{
	using SDL_GLContext = void*;
	class Window
	{
	public:
		Window();
		~Window();
		
		bool create();
		void shutdown();

		static bool initialize();

		void pumpEvents();

		static void checkSDLVersion();
		void getWindowSize(int& w, int& h);

		SDL_Window* getSDLWindow() { return mWindow; }
		SDL_GLContext getGLContext() { return mGLContext; }

		void beginRender();
		void endRender();

	private:
		SDL_Window* mWindow;
		SDL_GLContext mGLContext;
		ImguiWindow mImguiwindow;
	};
}