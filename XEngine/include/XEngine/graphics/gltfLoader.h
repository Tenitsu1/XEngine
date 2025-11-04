//#pragma once
//
//#include <map>
//#include <memory>
//#include <string>
//
////#include "stb_image.h"
//typedef unsigned int GLuint;
//
//namespace tinygltf
//{
//	class Model;
//	class Node;
//	struct Mesh;
//}
//
//namespace XEngine::graphics
//{
//
//	class GLTFStaticMesh
//	{
//	public:
//
//		GLTFStaticMesh(tinygltf::Model &model, const char* filename);
//	/*	~GLTFStaticMesh();*/
//
//
//		std::pair<GLuint, std::map<int, GLuint>> bindModel(tinygltf::Model& model);
//		void dbgModel(tinygltf::Model& model);
//		/*std::pair<unsigned int, std::map<int, unsigned int>> bindModel();
//
//		void prepareForDrawing();
//		void draw();*/
//		
//	private:
//		void bindMesh(std::map<int, GLuint>& vbos, tinygltf::Model& model, tinygltf::Mesh& mesh);
//		void bindModelNodes(std::map<int, GLuint>& vbos, tinygltf::Model& model, tinygltf::Node& node);
//		
//		void drawMesh(const std::map<int, GLuint>& vbos, tinygltf::Model& model, tinygltf::Mesh& mesh);
//		void drawModelNodes(const std::pair<GLuint, std::map<int, GLuint>>& VAO_and_EBOs, tinygltf::Model& model, tinygltf::Node& node);
//		void drawModel(const std::pair<GLuint, std::map<int, GLuint>>& vaoAndEbos, tinygltf::Model& model);
//		/*void bindModelNodes(std::map<int, unsigned int>& mEbos, tinygltf::Node& node);
//		void bindMesh(std::map<int, unsigned int>& mEbos, tinygltf::Mesh& mesh);
//		void drawModelNodes(tinygltf::Node& node);
//		void drawMesh(const std::map<int, unsigned int>& mEbos, tinygltf::Mesh& mesh);*/
//	};
//	
//
//}