#pragma once

typedef union SDL_Event SDL_Event;

namespace XEngine::core
{
	class ImguiWindow
	{
	public:
		void create();
		void shutdown();

		void handleSDLEvent(SDL_Event& e);

		void beginRender();
		void endRender();
	};

}