#include "graphics/rendercommands.h"
#include "log.h"

#include "graphics/mesh.h"
#include "graphics/shader.h"
#include "graphics/framebuffer.h"
#include "engine.h"

#include "glad/glad.h"

namespace XEngine::graphics::rendercommands
{
	void RenderMesh::execute()
	{
		std::shared_ptr<Mesh> mesh = mMesh.lock();
		std::shared_ptr<Shader> shader = mShader.lock();
		if (mesh && shader)
		{
			mesh->bind();
			shader->bind();

			if (mesh->getElementCount() > 0)
			{
				glDrawElements(GL_TRIANGLES, mesh->getElementCount(), GL_UNSIGNED_INT, 0);
			}
			else
			{
				glDrawArrays(GL_TRIANGLE_STRIP,0, mesh->getVertexCount());
			}
			

			shader->unbind();
			mesh->unbind();
		}
		else
		{
			XENGINE_WARN("Attempting to execute RenderMesh with invalid data");
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
