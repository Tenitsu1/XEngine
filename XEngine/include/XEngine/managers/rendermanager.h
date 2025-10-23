#pragma once

#include "graphics/rendercommands.h"
#include <queue>

namespace XEngine::managers
{
	class RenderManager
	{
	public:
		RenderManager() {};
		~RenderManager() {};

		void initialize();
		void shutdown();

		void clear();
		void setClearColor(float r, float g, float b, float a);

		void submit(std::unique_ptr<graphics::rendercommands::RenderCommand> rc);

		// Execute submitted RenderCommands _in the order they were received_.
		// We can extend the API if we need to mitigate performance impact.
		void fulsh();

	private:
		std::queue<std::unique_ptr<graphics::rendercommands::RenderCommand>> mRenderCommands;
	};

}