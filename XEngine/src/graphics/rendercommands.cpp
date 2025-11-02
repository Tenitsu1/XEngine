#include "graphics/rendercommands.h"
#include "log.h"

#include "graphics/vertexbuffer.h"
#include "graphics/shader.h"
#include "graphics/texture.h"
#include "graphics/framebuffer.h"
#include "engine.h"

#include "glad/glad.h"

namespace XEngine::graphics::rendercommands
{
	void RenderVertexArray::execute()
	{
		std::shared_ptr<VertexArray> vertexArray = mVertexArray.lock();
		std::shared_ptr<Shader> shader = mShader.lock();
		if (vertexArray && shader)
		{
			vertexArray->bind();
			shader->bind();

			if (vertexArray->getElementCount() > 0)
			{
				glDrawElements(GL_TRIANGLES, vertexArray->getElementCount(), GL_UNSIGNED_INT, 0);
			}
			else
			{
				glDrawArrays(GL_TRIANGLE_STRIP,0, vertexArray->getVertexCount());
			}
			

			shader->unbind();
			vertexArray->unbind();
		}
		else
		{
			XENGINE_WARN("Attempting to execute RenderVertexArray with invalid data");
		}
	}

	void RenderVertexArrayTexture::execute()
	{
		std::shared_ptr<VertexArray> vertexArray = mVertexArray.lock();
		std::shared_ptr<Texture> texture = mTexture.lock();
		std::shared_ptr<Shader> shader = mShader.lock();
		if (vertexArray && shader)
		{
			vertexArray->bind();
			texture->bind();
			shader->bind();

			if (vertexArray->getElementCount() > 0)
			{
				glDrawElements(GL_TRIANGLES, vertexArray->getElementCount(), GL_UNSIGNED_INT, 0);
			}
			else
			{
				glDrawArrays(GL_TRIANGLE_STRIP, 0, vertexArray->getVertexCount());
			}


			shader->unbind();
			texture->unbind();
			vertexArray->unbind();
		}
		else
		{
			XENGINE_WARN("Attempting to execute RenderVertexArrayTexture with invalid data");
		}
	}


	void PushFramebuffer::execute()
	{
		std::shared_ptr<Framebuffer> fb = mFramebuffer.lock();
		if (fb)
		{
			Engine::Instance().getRenderManager().pushFramebuffer(fb);
		}
		else
		{
			XENGINE_WARN("Attempting to execute PushFramebuffer with invalid data")
		}
	}

	void PopFramebuffer::execute()
	{
		Engine::Instance().getRenderManager().popFramebuffer();
	}
	
}
