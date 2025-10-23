#include "managers/rendermanager.h"

#include "log.h"

#include "glad/glad.h"
#include "graphics/helper.h "

namespace XEngine::managers
{
	void RenderManager::initialize()
	{
		XENGINE_INFO("OpenGL Info:\n Vendor:\t{}\n Renderer:\t{}\n Version:\t{}",
			(const char*)glGetString(GL_VENDOR),
			(const char*)glGetString(GL_RENDERER),
			(const char*)glGetString(GL_VERSION));

		// Initialize OpenGL
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA);

		/*setClearColor(
			static_cast<float>(0x64) / static_cast<float>(0xFF),
			static_cast<float>(0x95) / static_cast<float>(0xFF),
			static_cast<float>(0xED) / static_cast<float>(0xFF),
			1
		);*/

	}

	void RenderManager::shutdown()
	{
		while (mRenderCommands.size() > 0)
		{
			mRenderCommands.pop();
		}
	}

	void RenderManager::setClearColor(float r, float g, float b, float a)
	{
		glClearColor(r, g, b, a);

	}

	void RenderManager::submit(std::unique_ptr<graphics::rendercommands::RenderCommand> rc)
	{
		mRenderCommands.push(std::move(rc));
	}

	void RenderManager::fulsh()
	{
		while (mRenderCommands.size() > 0)
		{
			auto rc = std::move(mRenderCommands.front());
			mRenderCommands.pop();

			rc->execute();
		}
	}

	void RenderManager::clear()
	{
		XENGINE_ASSERT(mRenderCommands.size() == 0, "Unflushed render commands in queue!");
		while (!mRenderCommands.empty())
		{
			mRenderCommands.pop();
		}
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

}