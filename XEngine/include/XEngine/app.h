#pragma once
#include "core/window.h"

namespace XEngine
{
	class App
	{
	public:
		App() {}
		~App() {}

		virtual core::WindowProperties getWindowProperties() { return core::WindowProperties(); }

		virtual void initialize() {};
		virtual void shutdown() {};
		virtual void update() {};
		virtual void render() {};
		virtual void imguiRender() {};

	private:

	};
}