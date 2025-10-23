#include "graphics/mesh.h"
//#include "graphics/helper.h"
#include "log.h"

#include "glad/glad.h"


namespace XEngine::graphics
{
	// VAO
	Mesh::Mesh(float* vertexArray, uint32_t vertexCount, uint32_t dimensions)
		: mVertexCount(vertexCount)
		, mEbo(0)
		, mElementCount(0)
	{
		glGenVertexArrays(1, &mVao);
		if (mVao == 0) {
			XENGINE_ERROR("Failed to generate vertex array");
			return;
		}

		glBindVertexArray(mVao);

		glGenBuffers(1, &mPositionVbo);
		if (mPositionVbo == 0) {
			XENGINE_ERROR("Failed to generate vertex buffer");
			return;
		}

		glBindBuffer(GL_ARRAY_BUFFER, mPositionVbo);
		glBufferData(GL_ARRAY_BUFFER, vertexCount * dimensions * sizeof(float), vertexArray, GL_STATIC_DRAW);
		if (glGetError() != GL_NO_ERROR) {
			XENGINE_ERROR("Failed to upload vertex data");
			return;
		}

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, dimensions, GL_FLOAT, GL_FALSE, dimensions * sizeof(float), (void*)0);
		glDisableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glBindVertexArray(0);
	}


	// EBO
	Mesh::Mesh(float* vertexArray, uint32_t vertexCount, uint32_t dimensions, uint32_t* elementArray, uint32_t elementCount)
		: Mesh(vertexArray, vertexCount, dimensions)
	{
		mElementCount = elementCount;
		glBindVertexArray(mVao);

		glGenBuffers(1, &mEbo);
		if (mEbo == 0) {
			XENGINE_ERROR("Failed to generate element buffer");
			return;
		}

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mEbo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, elementCount * sizeof(uint32_t), elementArray, GL_STATIC_DRAW);
		if (glGetError() != GL_NO_ERROR) {
			XENGINE_ERROR("Failed to upload element data");
			return;
		}

		glBindVertexArray(0);
	}

	Mesh::~Mesh()
	{
		glDeleteBuffers(1, &mPositionVbo);
		glDeleteBuffers(1, &mEbo);
		glDeleteVertexArrays(1, &mVao);
	}


	//Mesh::Mesh(float* vertexArray, uint32_t vertexCount, uint32_t dimensions) 
	//	: mVertexCount(vertexCount)
	//	, mEbo(0)
	//	, mElementCount(0)
	//{
	//	glGenVertexArrays(1, &mVao); 
	//	glBindVertexArray(mVao); 

	//	glGenBuffers(1, &mPositionVbo); 
	//	glBindBuffer(GL_ARRAY_BUFFER, mPositionVbo); 
	//	glBufferData(GL_ARRAY_BUFFER, vertexCount * dimensions * sizeof(float),vertexArray, GL_STATIC_DRAW); 

	//	glEnableVertexAttribArray(0); 
	//	glVertexAttribPointer(0, dimensions, GL_FLOAT, GL_FALSE, 0, 0); 
	//	glDisableVertexAttribArray(0); 
	//	glBindBuffer(GL_ARRAY_BUFFER, 0); 

	//	glBindVertexArray(0); 
	//}

	//Mesh::Mesh(float* vertexArray, uint32_t vertexCount, uint32_t dimensions, uint32_t* elementArray, uint32_t elementCount)
	//	: Mesh(vertexArray, vertexCount, dimensions)
	//{
	//	mElementCount = elementCount;
	//	glBindVertexArray(mVao);

	//	glGenBuffers(1, &mEbo);
	//	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mEbo);
	//	glBufferData(GL_ELEMENT_ARRAY_BUFFER, elementCount * sizeof(uint32_t), elementArray, GL_STATIC_DRAW);

	//	glBindVertexArray(0);
	//}

	//Mesh::~Mesh()
	//{
	//	glDeleteBuffers(1, &mPositionVbo); 
	//	if (mEbo != 0)
	//	{
	//		glDeleteBuffers(1, &mEbo);
	//	}
	//	glDeleteVertexArrays(1, &mVao); 
	//}

	void Mesh::bind()
	{
		glBindVertexArray(mVao); 
		glEnableVertexAttribArray(0); 
	}

	void Mesh::unbind()
	{
		glDisableVertexAttribArray(0); 
		glBindVertexArray(0); 
	}
}