#pragma once
#include <vector>

namespace XEngine::graphics
{
	class RawVertexBuffer
	{
	public:
		RawVertexBuffer();
		virtual ~RawVertexBuffer();

		virtual uint32_t getTypeSize() const = 0;
		inline bool isUploaded() const { return mIsUploaded; }
		inline uint32_t getVbo() const { return mVbo; }
		inline uint32_t getVertexCount() const { return mVertexCount; }
		inline uint32_t getStride() const { return mStride; }
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
	};

	template<typename T>
	class VertexBuffer : public RawVertexBuffer
	{
	public:
		VertexBuffer() {};
		~VertexBuffer() {};

		uint32_t getTypeSize() const override { return sizeof(T); }

		void pushVertex(const std::vector<T>& vert)
		{
			mVertexCount++;
			mDataVec.insert(mDataVec.end(), vert.begin(), vert.end());
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
	};

	class VertexArray
	{
	public:
		VertexArray();
		~VertexArray();

		inline bool isValue() const { return mIsVaild; }
		inline uint32_t getVertexCount() const { return	mVertexCount; }
		inline uint32_t getElementCount() const { return mElementCount; }

		void pushBuffer(RawVertexBuffer* vbo);
		void setElement(const std::vector<uint32_t>& element);

		void upload();

		void bind();
		void unbind();

	private:
		bool mIsVaild;
		uint32_t mVertexCount, mElementCount;
		uint32_t mVao, mEbo;
		uint32_t mAttributeCount;
		std::vector<RawVertexBuffer*> mVbos;
	};
}