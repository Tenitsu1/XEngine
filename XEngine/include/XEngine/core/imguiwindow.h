#pragma once

typedef union SDL_Event SDL_Event;

namespace XEngine::core
{
	struct ImguiWindowProperties
	{
		bool moveFromTitleBarOnly = true;
		bool isDockingEnable = false;
		bool isViewportEnable = false;
	};

	class ImguiWindow
	{
	public:
		void create(const ImguiWindowProperties& props);
		void shutdown();

		void handleSDLEvent(SDL_Event& e);

		bool wantCaptureMouse();
		bool wantCaptureKeyboard();

		void beginRender();
		void endRender();
	};

}