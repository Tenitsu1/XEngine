#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>
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

struct Triangle
{
	// vertices
	glm::vec3 v1;       //offset 0   // alignment 16 // size 12 // total 12 bytes
	float padding1;     //offset 12  // alignment 4  // size 4  // total 16 bytes
	glm::vec3 v2;       //offset 16  // alignment 16 // size 12 // total 28 bytes
	float padding2;     //offset 28  // alignment 4  // size 4  // total 32 bytes 
	glm::vec3 v3;       //offset 32  // alignment 16 // size 12 // total 44 bytes
	float padding3;     //offset 44  // alignment 4  // size 4  // total 48 bytes

	// normals
	glm::vec3 NA;       //offset 48  // alignment 16 // size 12 // total 60 bytes
	float padding4;     //offset 60  // alignment 4  // size 4  // total 64 bytes
	glm::vec3 NB;       //offset 64  // alignment 16 // size 12 // total 76 bytes
	float padding5;     //offset 76  // alignment 4  // size 4  // total 80 bytes
	glm::vec3 NC;       //offset 80  // alignment 16 // size 12 // total 92 bytes
	float padding6;     //offset 92  // alignment 4  // size 4  // total 96 bytes
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

		inline const std::vector<Triangle>& getTriangles() const { return mTriangles; }

	private:
		std::vector<Triangle> mTriangles;
		void extractTriangles(tinygltf::Model& model);
		void extractNodeTriangles(tinygltf::Model& model, const tinygltf::Node& node, const glm::mat4& parentTransform);

	private:
		void bindMesh(std::map<int, GLuint>& vbos, tinygltf::Model& model, tinygltf::Mesh& mesh);
		void bindModelNodes(std::map<int, GLuint>& vbos, tinygltf::Model& model, tinygltf::Node& node);

		void drawMesh(const std::map<int, GLuint>& vbos, tinygltf::Model& model, tinygltf::Mesh& mesh);
		void drawModelNodes(const std::pair<GLuint, std::map<int, GLuint>>& VAO_and_EBOs, tinygltf::Model& model, tinygltf::Node& node);

	private:
		std::pair<GLuint, std::map<int, GLuint>> vaoAndEbos;
		std::weak_ptr<Shader> mShader;
		/*void bindModelNodes(std::map<int, unsigned int>& mEbos, tinygltf::Node& node);
		void bindMesh(std::map<int, unsigned int>& mEbos, tinygltf::Mesh& mesh);
		void drawModelNodes(tinygltf::Node& node);
		void drawMesh(const std::map<int, unsigned int>& mEbos, tinygltf::Mesh& mesh);*/
	};


}