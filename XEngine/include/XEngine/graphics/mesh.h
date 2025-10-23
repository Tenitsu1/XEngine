#pragma once

#include <cstdint>

namespace XEngine::graphics
{
	class Mesh
	{
	public:
		Mesh(float* vertexArray, uint32_t vertexCount, uint32_t dimensions);
		Mesh(float* vertexArray, uint32_t vertexCount, uint32_t dimensions, uint32_t* elementArray,  uint32_t elementCount);
		~Mesh();

		void bind();
		void unbind();

		inline uint32_t getVertexCount() const { return mVertexCount; }
		inline uint32_t getElementCount() const { return mElementCount; }

	private:
		uint32_t mVertexCount;
		uint32_t mVao;
		uint32_t mElementCount;
		uint32_t mEbo;
		uint32_t mPositionVbo;
	};
}