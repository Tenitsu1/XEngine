#include "managers/rendermanager.h"

#include "log.h"

#include "glad/glad.h"
#include "graphics/helper.h "
#include "graphics/framebuffer.h"

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

	void RenderManager::setClearColor(const glm::vec4 clearColor)
	{
		glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);

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

	void RenderManager::pushFramebuffer(std::shared_ptr<graphics::Framebuffer> framebuffer)
	{
		mFramebuffers.push(framebuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer->getFbo());

		auto clearColor = framebuffer->getClearColor();
		glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	}

	void RenderManager::popFramebuffer()
	{
		XENGINE_ASSERT(mFramebuffers.size() > 0, "RenderManager::popFramebuffer - empty stack");
		if (mFramebuffers.size()>0)
		{
			mFramebuffers.pop();
			if (mFramebuffers.size() > 0)
			{
				auto nextfb = mFramebuffers.top();
				glBindFramebuffer(GL_FRAMEBUFFER, nextfb->getFbo());
			}
			else
			{
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
			}
		}
	}

}