#include "graphics/gltfLoader.h"

#define TINYGLTF_IMPLEMENTATION
//#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
//#define STBI_MSC_SECURE_CRT
#define TINYGLTF_NOEXCEPTION
#define JSON_NOEXCEPTION
#include "external/tinygltf/tiny_gltf.h"
#include "log.h"
#include "glad/glad.h"


namespace XEngine::graphics
{

	tinygltf::Model model;

	GLTFStaticMesh::GLTFStaticMesh(const char* filename)
	{
		tinygltf::TinyGLTF loader;
		std::string error;
		std::string warning;

		bool result = loader.LoadASCIIFromFile(&model, &error, &warning, filename);

		if (!warning.empty())
		{
			XENGINE_WARN("Warning: {}", warning);
		}

		if (!error.empty())
		{
			XENGINE_ERROR("Error : {}", error);
		}
		
		if (!result)
		{
			XENGINE_ERROR("Failed to load glTF : ", filename);
		}
		else
		{
			XENGINE_TRACE("Loaded gltf : {}", filename);
		}

		VAO_and_EBOs = bindModel();
	}

	GLTFStaticMesh::~GLTFStaticMesh()
	{
		glDeleteVertexArrays(1, &VAO_and_EBOs.first);

		for (auto item = VAO_and_EBOs.second.cbegin(); item != VAO_and_EBOs.second.cend();)
		{
			glDeleteBuffers(1, &VAO_and_EBOs.second[item->first]);
			VAO_and_EBOs.second.erase(item++);
		}
	}

	std::pair<unsigned int, std::map<int, unsigned int>> GLTFStaticMesh::bindModel()
	{
		std::map<int, unsigned int> mEbos;
		uint32_t mVao;
		glGenVertexArrays(1, &mVao);
		glBindVertexArray(mVao);

		const tinygltf::Scene& scene = model.scenes[model.defaultScene];
		for (size_t i = 0; i < scene.nodes.size(); ++i)
		{
			assert((scene.nodes[i] >= 0) && (scene.nodes[i] < model.nodes.size()));
			bindModelNodes(mEbos, model.nodes[scene.nodes[i]]);
		}
		glBindVertexArray(0);

		for (auto item = mEbos.cbegin(); item != mEbos.cend();)
		{
			tinygltf::BufferView bufferView = model.bufferViews[item->first];
			if (bufferView.target != GL_ELEMENT_ARRAY_BUFFER)
			{
				glDeleteBuffers(1, &mEbos[item->first]);
				mEbos.erase(item++);
			}
			else
			{
				++item;
			}
		}

		return { mVao, mEbos };
	}

	void GLTFStaticMesh::bindModelNodes(std::map<int, unsigned int>& mEbos, tinygltf::Node& node)
	{
		if ((node.mesh >= 0) && (node.mesh < model.meshes.size()))
		{
			bindMesh(mEbos, model.meshes[node.mesh]);
		}

		for (size_t i = 0; i < node.children.size(); i++)
		{
			assert((node.children[i] >= 0) && (node.children[i] < model.nodes.size()));
			bindModelNodes(mEbos, model.nodes[node.children[i]]);
		}
	}

	void GLTFStaticMesh::bindMesh(std::map<int, unsigned int>& mEbos, tinygltf::Mesh& mesh)
	{
		for (int i = 0; i < model.bufferViews.size(); ++i)
		{
			const tinygltf::BufferView& bufferView = model.bufferViews[i];
			if (bufferView.target == 0)
			{
				continue;
			}

			const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

			GLuint mEbo;
			glGenBuffers(1, &mEbo);
			mEbos[i] = mEbo;
			glBufferData(
				bufferView.target, bufferView.byteLength,
				&buffer.data.at(0) + bufferView.byteOffset, GL_STATIC_DRAW
			);

			for (size_t i = 0; i < mesh.primitives.size(); ++i)
			{
				tinygltf::Primitive primitive = mesh.primitives[i];
				tinygltf::Accessor indexAccessor = model.accessors[primitive.indices];

				for (auto& attrib : primitive.attributes)
				{
					tinygltf::Accessor accessor = model.accessors[attrib.second];
					int byteStride = accessor.ByteStride(model.bufferViews[accessor.bufferView]);
					glBindBuffer(GL_ARRAY_BUFFER, mEbos[accessor.bufferView]);

					int size = 1;
					if (accessor.type != TINYGLTF_TYPE_SCALAR)
					{
						size = accessor.type;
					}

					int attribute = -1;
					if (attrib.first.compare("POSITION") == 0) { attribute = 0; }
					if (attrib.first.compare("TEXCOORD_0") == 0) { attribute = 1; }
					if (attrib.first.compare("NORMAL") == 0) { attribute = 2; }
					if (attribute >= 0)
					{
						glEnableVertexAttribArray(attribute);
						glVertexAttribPointer(
							attribute, size, accessor.componentType, accessor.normalized ? GL_TRUE : GL_FALSE,
							byteStride, (char*)NULL + accessor.byteOffset
						);
					}

				}
			}
		}
	}

	/*void GLTFStaticMesh::prepareForDrawing()
	{ 
		glBindVertexArray(VAO_and_EBOs.first);
	}

	void GLTFStaticMesh::draw() 
	{
		const tinygltf::Scene& scene = model.scenes[model.defaultScene];
		for (size_t i = 0; i < scene.nodes.size(); ++i)
		{
			drawModelNodes(model.nodes[scene.nodes[i]]);
		}
	}

	void GLTFStaticMesh::drawModelNodes(tinygltf::Node& node)
	{
		if ((node.mesh >= 0) && (node.mesh < model.meshes.size()))
		{
			drawMesh(VAO_and_EBOs.second, model.meshes[node.mesh]);
		}
		for (size_t i = 0; i < node.children.size(); ++i)
		{
			drawModelNodes(model.nodes[node.children[i]]);
		}
	}

	void GLTFStaticMesh::drawMesh(const std::map<int, unsigned int>& mEbos, tinygltf::Mesh& mesh) 
	{
		for (int i = 0; i < mesh.primitives.size(); ++i)
		{
			tinygltf::Primitive& primitive = mesh.primitives[i];
			tinygltf::Accessor& indexAccessor = model.accessors[primitive.indices];

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mEbos.at(indexAccessor.bufferView));
			glDrawElements(
				primitive.mode, indexAccessor.count,
				indexAccessor.componentType,
				(char*)NULL + indexAccessor.byteOffset
			);
		}
	}*/
}