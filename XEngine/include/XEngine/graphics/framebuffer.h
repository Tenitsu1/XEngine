#pragma once

#include <cstdint>

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
		inline void getSize(uint32_t& w, uint32_t& h) { w = mWidth; h = mHeight; }
		inline void setClearColor(float r, float g, float b, float a) { mCCR = r; mCCG = g; mCCB = b; mCCA = a;}
		inline void getClearColor(float& r, float& g, float& b, float& a) { r = mCCR; g = mCCG; b = mCCB; a = mCCA; }

	private:
		uint32_t mFbo;
		uint32_t mTextureId;
		uint32_t mRenderbuffer;

		uint32_t mWidth, mHeight;
		float mCCR, mCCG, mCCB, mCCA;
	};
}