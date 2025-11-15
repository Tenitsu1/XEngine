#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>
#include "structs.hpp"
#include "external/glm/glm.hpp"

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
	class Shader;

	class GLTFStaticMesh
	{
	public:
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
		std::pair<GLuint, std::map<int, GLuint>> bindModel(tinygltf::Model& model);
		void dbgModel(tinygltf::Model& model);
		/*std::pair<unsigned int, std::map<int, unsigned int>> bindModel();

		void prepareForDrawing();
		void draw();*/

		inline std::vector<Triangle>& getTriangles() { return mTriangles; }
		inline Mesh& getMesh() { return mMesh; }
		inline const std::vector<GLuint>& getTextures() const { return mTextures; }

	private:
		std::vector<Triangle> mTriangles;
		Mesh mMesh;
		void extractTriangles(tinygltf::Model& model);
		void extractNodeTriangles(tinygltf::Model& model, const tinygltf::Node& node, const glm::mat4& parentTransform);
		void extractMesh(tinygltf::Model& model);
		void extractNodeMesh(tinygltf::Model& model, const tinygltf::Node& node, const glm::mat4& parentTransform);

	private:
		void bindMesh(std::map<int, GLuint>& vbos, tinygltf::Model& model, tinygltf::Mesh& mesh);
		void bindModelNodes(std::map<int, GLuint>& vbos, tinygltf::Model& model, tinygltf::Node& node);

		void drawMesh(const std::map<int, GLuint>& vbos, tinygltf::Model& model, tinygltf::Mesh& mesh);
		void drawModelNodes(const std::pair<GLuint, std::map<int, GLuint>>& VAO_and_EBOs, tinygltf::Model& model, tinygltf::Node& node);

		void loadTextures(tinygltf::Model& model);

	private:
		std::pair<GLuint, std::map<int, GLuint>> vaoAndEbos;
		std::vector<GLuint> mTextures;
		std::weak_ptr<Shader> mShader;
		/*void bindModelNodes(std::map<int, unsigned int>& mEbos, tinygltf::Node& node);
		void bindMesh(std::map<int, unsigned int>& mEbos, tinygltf::Mesh& mesh);
		void drawModelNodes(tinygltf::Node& node);
		void drawMesh(const std::map<int, unsigned int>& mEbos, tinygltf::Mesh& mesh);*/
	};


}