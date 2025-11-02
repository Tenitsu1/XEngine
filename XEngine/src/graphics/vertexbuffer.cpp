#include "graphics/vertexbuffer.h"
#include "log.h"

#include "glad/glad.h"



namespace XEngine::graphics
{
	RawVertexBuffer::RawVertexBuffer()
	{
		glGenBuffers(1, &mVbo);
	}

	RawVertexBuffer::~RawVertexBuffer()
	{
		glDeleteBuffers(1, &mVbo);
	}

	void RawVertexBuffer::setLayout(const std::vector<uint32_t>& layout)
	{
		mLayout = layout;
		mStride = 0;
		for (auto& count : layout)
		{
			mStride += count;
		}
	}

	void RawVertexBuffer::upload(bool dynamic /*false*/)
	{
		glBindBuffer(GL_ARRAY_BUFFER, mVbo);
		glBufferData(GL_ARRAY_BUFFER, mSize, mData, dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		mIsUploaded = true;
	}

	void RawVertexBuffer::bind()
	{
		glBindBuffer(GL_ARRAY_BUFFER, mVbo);
	}

	void RawVertexBuffer::unbind()
	{
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	VertexArray::VertexArray()
		: mVao(0)
		, mEbo(0)
		, mAttributeCount(0)
		, mVertexCount(0)
		, mElementCount(0)
		, mIsVaild(false)
	{
		glGenVertexArrays(1, &mVao);
	}

	VertexArray::~VertexArray()
	{
		uint32_t id;
		for (auto& vbo : mVbos )
		{
			id = vbo->getVbo();
			glDeleteBuffers(1, &id);
			delete vbo;
		}
		glDeleteVertexArrays(1, &mVao);
		mVbos.clear();
	}

	void VertexArray::pushBuffer(RawVertexBuffer* vbo)
	{
		if (mVbos.size() > 0)
		{
			XENGINE_ASSERT(mVbos[0]->getVertexCount() == vbo->getVertexCount(), "VertexArray::PushBuffer - Attempying to push a VertexBuffer with a different VertexCount.");
		}
		XENGINE_ASSERT(vbo->getLayout().size() > 0, "VertexArray::PushBuffer - VertexBuffer has no layout defined.");
		if (vbo->getLayout().size() > 0)
		{
			mVbos.push_back(vbo);
			mVertexCount = (uint32_t)mVbos[0]->getVertexCount();
		}
		
	}

	void VertexArray::setElement(const std::vector<uint32_t>& element)
	{
		mElementCount = (uint32_t)element.size();
		glBindVertexArray(mVao);
		glGenBuffers(1, &mEbo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mEbo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, element.size() * sizeof(uint32_t), &element[0], GL_STATIC_DRAW);
		glBindVertexArray(0);
	}

	void VertexArray::upload()
	{
		glBindVertexArray(mVao);
		uint32_t attribute = 0;
		for (auto& vbo: mVbos)
		{
			if (!vbo->isUploaded())
			{
				vbo->upload(false);
			}
			vbo->bind();
			uint32_t offset = 0 ;
			for (uint32_t count : vbo->getLayout())
			{
				glEnableVertexAttribArray(attribute);
				glVertexAttribPointer(
					attribute, count, GL_FLOAT, GL_FALSE,
					vbo->getStride(), (void*)(intptr_t)offset);

				attribute++;
				offset += (count * vbo->getTypeSize());
			}
			vbo->unbind();
		}
		glBindVertexArray(0);
		mIsVaild = true;
	}

	void VertexArray::bind()
	{
		glBindVertexArray(mVao);
	}

	void VertexArray::unbind()
	{
		glBindVertexArray(0);
	}
}