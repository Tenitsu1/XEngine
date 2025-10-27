#pragma once

#include "core/imguiwindow.h"
#include <string>
#include <memory>

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
		float ccR, ccG, ccB;
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
		void getWindowSize(int& w, int& h);

		inline SDL_Window* getSDLWindow() { return mWindow; }
		inline SDL_GLContext getGLContext() { return mGLContext; }
		inline graphics::Framebuffer* getFramebuffer() { return mFramebuffer.get(); }

		void beginRender();
		void endRender();

	private:
		SDL_Window* mWindow;
		SDL_GLContext mGLContext;
		ImguiWindow mImguiwindow;
		std::shared_ptr<graphics::Framebuffer> mFramebuffer;
	};
}