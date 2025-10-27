#pragma once

#include "graphics/rendercommands.h"
#include <queue>
#include <stack>
#include <memory>

namespace XEngine::managers
{
	class RenderManager
	{
		friend class graphics::rendercommands::PushFramebuffer;
		friend class graphics::rendercommands::PopFramebuffer;

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
		void pushFramebuffer(std::shared_ptr<graphics::Framebuffer> framebuffer);
		void popFramebuffer();

	private:
		std::queue<std::unique_ptr<graphics::rendercommands::RenderCommand>> mRenderCommands;
		std::stack<std::shared_ptr<graphics::Framebuffer>> mFramebuffers;
	};

}