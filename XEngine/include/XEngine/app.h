#pragma once

namespace XEngine
{
	class App
	{
	public:
		App() {}
		~App() {}

		virtual void initialize() {};
		virtual void shutdown() {};
		virtual void update() {};
		virtual void render() {};
		virtual void imguiRender() {};

	private:

	};
}