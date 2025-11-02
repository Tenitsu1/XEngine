#pragma once

#include <map>
#include <memory>
#include <string>

//#include "stb_image.h"

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
		std::pair<unsigned int, std::map<int, unsigned int>> VAO_and_EBOs;

		GLTFStaticMesh(const char* filename);
		~GLTFStaticMesh();

		std::pair<unsigned int, std::map<int, unsigned int>> bindModel();

		void prepareForDrawing();
		void draw();

	private:
		void bindModelNodes(std::map<int, unsigned int>& mEbos, tinygltf::Node& node);
		void bindMesh(std::map<int, unsigned int>& mEbos, tinygltf::Mesh& mesh);
		void drawModelNodes(tinygltf::Node& node);
		void drawMesh(const std::map<int, unsigned int>& mEbos, tinygltf::Mesh& mesh);
	};
	

}