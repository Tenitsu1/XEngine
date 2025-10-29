#pragma once

#include <cstdint>

#include "external/glm/glm.hpp"

namespace XEngine::graphics
{
	class Framebuffer
	{
	public:
		Framebuffer(uint32_t mWidth, uint32_t mHeight);
		~Framebuffer();

		inline uint32_t getFbo() const { return mFbo; }
		inline uint32_t getTextureId() const { return mTextureId; }
		inline uint32_t getRenderbuffer() const { return mRenderbuffer; }
		inline glm::ivec2& getSize() { return mSize; }
		inline void setClearColor(const glm::vec4& clearColor) { mClearColor = clearColor;}
		inline glm::vec4& getClearColor() { return mClearColor; }

	private:
		uint32_t mFbo;
		uint32_t mTextureId;
		uint32_t mRenderbuffer;

		glm::ivec2 mSize;
		glm::vec4 mClearColor;
		
	};
}