#pragma once

#include <vector>
#include <memory>
#include <type_traits>

namespace XEngine::graphics
{
	class RawVertexBuffer
	{
	public:
		static const uint32_t GLTypeByte;
		static const uint32_t GLTypeUByte;
		static const uint32_t GLTypeShort;
		static const uint32_t GLTypeUShort;
		static const uint32_t GLTypeInt;
		static const uint32_t GLTypeUint;
		static const uint32_t GLTypeFloat;
		static const uint32_t GLTypeDouble;

	public:
		RawVertexBuffer();
		virtual ~RawVertexBuffer();

		virtual uint32_t getTypeSize() const = 0;
		inline bool isUploaded() const { return mIsUploaded; }
		inline uint32_t getVbo() const { return mVbo; }
		inline uint32_t getVertexCount() const { return mVertexCount; }
		inline uint32_t getStride() const { return mStride; }
		inline uint32_t getGLType() const { return mGLType; }
		inline const std::vector<uint32_t>& getLayout() const { return mLayout; }

		void setLayout(const std::vector<uint32_t>& layout);

		virtual void upload(bool dynamic = false);

		void bind();
		void unbind();

	protected:
		bool mIsUploaded = false;
		uint32_t mVbo = 0;
		uint32_t mVertexCount = 0;
		uint32_t mStride = 0;
 
		std::vector<uint32_t> mLayout;
		void* mData = nullptr;
		uint32_t mSize = 0;
		uint32_t mGLType = 0;
	};

	template<typename T>
	class VertexBuffer : public RawVertexBuffer
	{
		static_assert(
			std::is_same<T, char>() ||              // GL_BYTE
			std::is_same<T, unsigned char>() ||    // GL_UNSIGNED_BYTE
			std::is_same<T, short>() ||            // GL_SHORT
			std::is_same<T, unsigned short>() ||   // GL_UNSIGNED_SHORT
			std::is_same<T, int>() ||              // GL_INT
			std::is_same<T, unsigned int>() ||     // GL_UNSINGNED_INT
			std::is_same<T, float>()        ||     // GL_FLOAT
			std::is_same<T, double>()              // GL_DOUBLE
			, "This type is not supported.");
	public:
		VertexBuffer()
			: mValueCount(0)
		{
			if constexpr (std::is_same<T, char>())              { mGLType = RawVertexBuffer::GLTypeByte; }
			if constexpr (std::is_same<T, unsigned char>())     { mGLType = RawVertexBuffer::GLTypeUByte; }
			if constexpr (std::is_same<T, short>())             { mGLType = RawVertexBuffer::GLTypeShort; }
			if constexpr (std::is_same<T, unsigned short>())    { mGLType = RawVertexBuffer::GLTypeUShort; }
			if constexpr (std::is_same<T, int>())               { mGLType = RawVertexBuffer::GLTypeInt; }
			if constexpr (std::is_same<T, unsigned int>())      { mGLType = RawVertexBuffer::GLTypeUint; }
			if constexpr (std::is_same<T, float>())             { mGLType = RawVertexBuffer::GLTypeFloat; }
			if constexpr (std::is_same<T, double>())            { mGLType = RawVertexBuffer::GLTypeDouble; }
		};
		~VertexBuffer() {};

		uint32_t getTypeSize() const override { return sizeof(T); }

		void pushVertex(const std::vector<T>& vert)
		{
			XENGINE_ASSERT(vert.size() > 0, "No value passed in for vertex");
			if (mDataVec.size() == 0)
			{
				mValueCount = (uint32_t)vert.size();
			}

			XENGINE_ASSERT(vert.size() == mValueCount, "Attempying to push a Vertex with an unexpected amount of values.");
	
			if (vert.size() == mValueCount)
			{
				mVertexCount++;
				mDataVec.insert(mDataVec.end(), vert.begin(), vert.end());
			}
		}

		void upload(bool dynaminc = false) override
		{
			mStride *= sizeof(T);
			mSize = sizeof(T) * (uint32_t)mDataVec.size();
			XENGINE_TRACE("VertexBuffer::Upload() - mSize : {}, mStride : {}", mSize, mStride);
			XENGINE_ASSERT(mSize > 0 ,"VertexBuffer::Upload() - mSize = 0");
			mData = &mDataVec[0];
			RawVertexBuffer::upload(dynaminc);
		}

	private:
		std::vector<T> mDataVec;
		uint32_t mValueCount;
	};

	class VertexArray
	{
	public:
		VertexArray();
		~VertexArray();

		inline bool isValue() const { return mIsValid; }
		inline uint32_t getVertexCount() const { return	mVertexCount; }
		inline uint32_t getElementCount() const { return mElementCount; }

		void pushBuffer(std::unique_ptr<RawVertexBuffer> vbo);
		void setElement(const std::vector<uint32_t>& element);

		void upload();

		void bind();
		void unbind();

	private:
		bool mIsValid;
		uint32_t mVertexCount, mElementCount;
		uint32_t mVao, mEbo;
		uint32_t mAttributeCount;
		std::vector<std::unique_ptr<RawVertexBuffer>> mVbos;
	};
}