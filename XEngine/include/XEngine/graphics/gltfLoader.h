#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>
#include "structs.hpp"
#include "external/glm/glm.hpp"

#include "XEngine/shaders/shader.h"

//#include "stb_image.h"
typedef unsigned int GLuint;

struct RaytracingMaterial
{
	// https://learnopengl.com/Advanced-OpenGL/Advanced-GLSL
	glm::vec3 color;              // offset 0   // alignment 16 // size 12 // total 12 bytes
	float emissionStrength;       // offset 12  // alignment 4  // size 4  // total 16 bytes
	glm::vec3 emissionColor;      // offset 16  // alignment 16 // size 12 // total 28 bytes
	float std140padding;          // offset 28  // alignment 4  // size 4  // total 32 bytes
};

namespace tinygltf
{
	class Model;
	class Node;
	struct Mesh;
}

namespace XEngine::graphics
{
	class GLTFStaticMesh
	{
	public:
		std::vector<PackedTriangle> mPackedTriangles;
		glm::vec3 getBoundsMin() const { return m_boundsMin; }
		glm::vec3 getBoundsMax() const { return m_boundsMax; }
	private:
		glm::vec3 m_boundsMin = glm::vec3(std::numeric_limits<float>::max());
		glm::vec3 m_boundsMax = glm::vec3(std::numeric_limits<float>::lowest());

	public:

		GLTFStaticMesh(tinygltf::Model& model, const char* filename);
		~GLTFStaticMesh();

		void bind();

		void drawModel(tinygltf::Model& model);
		std::map<int, GLuint> generateVBOs(tinygltf::Model& model);
		std::pair<GLuint, std::map<int, GLuint>> bindModel(tinygltf::Model& model);
		void dbgModel(tinygltf::Model& model);

		inline Mesh& getMesh() { return mMesh; }
		inline const uint32_t getTextureArrayID() { return mTextureArrayID; }
		inline const std::vector<GLuint>& getTextures() const { return mTextures; }
		inline const std::vector<Material>& getMaterials() const { return mMaterials; }
		inline const std::vector<PackedTriangle>& getPackedTriangles() const { return mPackedTriangles; }


		// Debug
		void printMaterialTextureMapping(const tinygltf::Model& model);
		void drawWithShader(std::shared_ptr<Shader> shader, tinygltf::Model& model);

		void createTextureArray(const tinygltf::Model& model);
	private:
		uint32_t mTextureArrayID;
		Mesh mMesh;
		std::vector<Material> mMaterials;;
		void extractMesh(tinygltf::Model& model);
		void extractNodeMesh(tinygltf::Model& model, const tinygltf::Node& node, const glm::mat4& parentTransform, unsigned int& vertex_offset);
		void extractMaterials(tinygltf::Model& model);

	private:
		void bindMesh(std::map<int, GLuint>& vbos, tinygltf::Model& model, tinygltf::Mesh& mesh);
		void bindModelNodes(std::map<int, GLuint>& vbos, tinygltf::Model& model, tinygltf::Node& node);

		void drawMesh(const std::map<int, GLuint>& vbos, tinygltf::Model& model, tinygltf::Mesh& mesh);
		void drawModelNodes(const std::pair<GLuint, std::map<int, GLuint>>& VAO_and_EBOs, tinygltf::Model& model, tinygltf::Node& node);

		void loadTextures(tinygltf::Model& model);

		void drawNodeRecursive(std::shared_ptr<Shader> shader, tinygltf::Model& model, int nodeIdx, const glm::mat4& parentTransform);
		void drawMeshWithMaterial(std::shared_ptr<Shader> shader, tinygltf::Model& model, tinygltf::Mesh& mesh);

		void setupRenderPrimitives(tinygltf::Model& model, std::map<int, GLuint>& bufferViewVBOs);

		void buildPackedTriangles();

	private:
		std::pair<GLuint, std::map<int, GLuint>> vaoAndEbos;
		std::map<int, GLuint> mVBOs;
		std::vector<GLuint> mTextures;

		std::map<int, std::vector<RenderPrimitive>> mRenderCache;

	};
}