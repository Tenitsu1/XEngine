#include "graphics/framebuffer.h"

#include "log.h"

#include "glad/glad.h"

namespace XEngine::graphics
{
	Framebuffer::Framebuffer(uint32_t width, uint32_t height)
		: mFbo(0)
		, mTextureId(0)
		, mRenderbuffer(0)
		, mSize({width, height})
		, mClearColor(1.f)
	{
		glGenFramebuffers(1, &mFbo);
		glBindFramebuffer(GL_FRAMEBUFFER, mFbo);


		
		/* When attaching a texture to a framebuffer,
		all rendering commands will write to the texture as
		if it was a normal color/depth or stencil buffer.
		The advantage of using textures is that the render output is
		stored inside the texture image
		that we can then easily use in our shaders.*/
		
		// Create texture
		
		glGenTextures(1, &mTextureId);
		glBindTexture(GL_TEXTURE_2D, mTextureId);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mSize.x, mSize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mTextureId, 0);

		// Create depth(24 bit)/stencil(8 bit) renderbuffer
		glGenRenderbuffers(1, &mRenderbuffer);
		glBindRenderbuffer(GL_RENDERBUFFER, mRenderbuffer);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, mSize.x, mSize.y);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, mRenderbuffer);

		// Check for completeness
		int32_t completeness = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (completeness != GL_FRAMEBUFFER_COMPLETE)
		{
			XENGINE_ERROR("Failure to create framebuffer. Complete status: {}", completeness);
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	Framebuffer::~Framebuffer()
	{
		glDeleteFramebuffers(1, &mFbo);
		mFbo = 0;
		mTextureId = 0;
		mRenderbuffer = 0;
	}
}