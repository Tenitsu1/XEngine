//#include "graphics/gltfLoader.h"
//
//#define TINYGLTF_IMPLEMENTATION
////#define STB_IMAGE_IMPLEMENTATION
//#define STB_IMAGE_WRITE_IMPLEMENTATION
////#define STBI_MSC_SECURE_CRT
//#define TINYGLTF_NOEXCEPTION
//#define JSON_NOEXCEPTION
//#include "external/tinygltf/tiny_gltf.h"
//#include "log.h"
//#include "glad/glad.h"
//
//#define BUFFER_OFFSET(i) ((char *)NULL + (i))
//
//
//namespace XEngine::graphics
//{
//
//	// tinygltf::Model model;
//
//	GLTFStaticMesh::GLTFStaticMesh(tinygltf::Model &model, const char* filename)
//	{
//		tinygltf::TinyGLTF loader;
//		std::string error;
//		std::string warning;
//
//		bool result = loader.LoadASCIIFromFile(&model, &error, &warning, filename);
//
//		if (!warning.empty())
//		{
//			XENGINE_WARN("Warning: {}", warning);
//		}
//
//		if (!error.empty())
//		{
//			XENGINE_ERROR("Error : {}", error);
//		}
//		
//		if (!result)
//		{
//			XENGINE_ERROR("Failed to load glTF : ", filename);
//		}
//		else
//		{
//			XENGINE_TRACE("Loaded gltf : {}", filename);
//		}
//	}
//
//	void GLTFStaticMesh::bindMesh(std::map<int, GLuint>& vbos,
//		tinygltf::Model& model, tinygltf::Mesh& mesh) {
//		for (size_t i = 0; i < model.bufferViews.size(); ++i) {
//			const tinygltf::BufferView& bufferView = model.bufferViews[i];
//			if (bufferView.target == 0) {  // TODO impl drawarrays
//				XENGINE_WARN("WARN: bufferView.target is zero");
//				continue;  // Unsupported bufferView.
//				/*
//				  From spec2.0 readme:
//				  https://github.com/KhronosGroup/glTF/tree/master/specification/2.0
//						   ... drawArrays function should be used with a count equal to
//				  the count            property of any of the accessors referenced by the
//				  attributes            property            (they are all equal for a given
//				  primitive).
//				*/
//			}
//
//			const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
//			XENGINE_TRACE("bufferview.target {} ", bufferView.target);
//
//			GLuint vbo;
//			glGenBuffers(1, &vbo);
//			vbos[i] = vbo;
//			glBindBuffer(bufferView.target, vbo);
//
//			XENGINE_TRACE("buffer.data.size = {}, bufferview.byteOffset = {}"
//				, buffer.data.size(), bufferView.byteOffset);
//
//			glBufferData(bufferView.target, bufferView.byteLength,
//				&buffer.data.at(0) + bufferView.byteOffset, GL_STATIC_DRAW);
//		}
//
//		for (size_t i = 0; i < mesh.primitives.size(); ++i) {
//			tinygltf::Primitive primitive = mesh.primitives[i];
//			tinygltf::Accessor indexAccessor = model.accessors[primitive.indices];
//
//			for (auto& attrib : primitive.attributes) {
//				tinygltf::Accessor accessor = model.accessors[attrib.second];
//				int byteStride =
//					accessor.ByteStride(model.bufferViews[accessor.bufferView]);
//				glBindBuffer(GL_ARRAY_BUFFER, vbos[accessor.bufferView]);
//
//				int size = 1;
//				if (accessor.type != TINYGLTF_TYPE_SCALAR) {
//					size = accessor.type;
//				}
//
//				int vaa = -1;
//				if (attrib.first.compare("POSITION") == 0) vaa = 0;
//				if (attrib.first.compare("NORMAL") == 0) vaa = 1;
//				if (attrib.first.compare("TEXCOORD_0") == 0) vaa = 2;
//				if (vaa > -1) {
//					glEnableVertexAttribArray(vaa);
//					glVertexAttribPointer(vaa, size, accessor.componentType,
//						accessor.normalized ? GL_TRUE : GL_FALSE,
//						byteStride, BUFFER_OFFSET(accessor.byteOffset));
//				}
//				else
//					XENGINE_WARN("vaa missing: {}", attrib.first);
//			}
//
//			if (model.textures.size() > 0) {
//				// fixme: Use material's baseColor
//				tinygltf::Texture& tex = model.textures[0];
//
//				if (tex.source > -1) {
//
//					GLuint texid;
//					glGenTextures(1, &texid);
//
//					tinygltf::Image& image = model.images[tex.source];
//
//					glBindTexture(GL_TEXTURE_2D, texid);
//					glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
//					glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//					glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
//					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
//
//					GLenum format = GL_RGBA;
//
//					if (image.component == 1) {
//						format = GL_RED;
//					}
//					else if (image.component == 2) {
//						format = GL_RG;
//					}
//					else if (image.component == 3) {
//						format = GL_RGB;
//					}
//					else {
//						// ???
//					}
//
//					GLenum type = GL_UNSIGNED_BYTE;
//					if (image.bits == 8) {
//						// ok
//					}
//					else if (image.bits == 16) {
//						type = GL_UNSIGNED_SHORT;
//					}
//					else {
//						// ???
//					}
//
//					glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width, image.height, 0,
//						format, type, &image.image.at(0));
//				}
//			}
//		}
//	}
//
//	// bind models
//	void GLTFStaticMesh::bindModelNodes(std::map<int, GLuint>& vbos, tinygltf::Model& model,
//		tinygltf::Node& node) {
//		if ((node.mesh >= 0) && (node.mesh < model.meshes.size())) {
//			bindMesh(vbos, model, model.meshes[node.mesh]);
//		}
//
//		for (size_t i = 0; i < node.children.size(); i++) {
//			assert((node.children[i] >= 0) && (node.children[i] < model.nodes.size()));
//			bindModelNodes(vbos, model, model.nodes[node.children[i]]);
//		}
//	}
//
//	std::pair<GLuint, std::map<int, GLuint>> GLTFStaticMesh::bindModel(tinygltf::Model& model) {
//		std::map<int, GLuint> vbos;
//		GLuint vao;
//		glGenVertexArrays(1, &vao);
//		glBindVertexArray(vao);
//
//		const tinygltf::Scene& scene = model.scenes[model.defaultScene];
//		for (size_t i = 0; i < scene.nodes.size(); ++i) {
//			assert((scene.nodes[i] >= 0) && (scene.nodes[i] < model.nodes.size()));
//			bindModelNodes(vbos, model, model.nodes[scene.nodes[i]]);
//		}
//
//		glBindVertexArray(0);
//		// cleanup vbos but do not delete index buffers yet
//		for (auto it = vbos.cbegin(); it != vbos.cend();) {
//			tinygltf::BufferView bufferView = model.bufferViews[it->first];
//			if (bufferView.target != GL_ELEMENT_ARRAY_BUFFER) {
//				glDeleteBuffers(1, &vbos[it->first]);
//				vbos.erase(it++);
//			}
//			else {
//				++it;
//			}
//		}
//
//		return { vao, vbos };
//	}
//
//	// recursively draw node and children nodes of model
//	void GLTFStaticMesh::drawModelNodes(const std::pair<GLuint, std::map<int, GLuint>>& vaoAndEbos,
//		tinygltf::Model& model, tinygltf::Node& node) {
//		if ((node.mesh >= 0) && (node.mesh < model.meshes.size())) {
//			drawMesh(vaoAndEbos.second, model, model.meshes[node.mesh]);
//		}
//		for (size_t i = 0; i < node.children.size(); i++) {
//			drawModelNodes(vaoAndEbos, model, model.nodes[node.children[i]]);
//		}
//	}
//
//
//	void GLTFStaticMesh::drawMesh(const std::map<int, GLuint>& vbos,
//		tinygltf::Model& model, tinygltf::Mesh& mesh) {
//		for (size_t i = 0; i < mesh.primitives.size(); ++i) {
//			tinygltf::Primitive primitive = mesh.primitives[i];
//			tinygltf::Accessor indexAccessor = model.accessors[primitive.indices];
//
//			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbos.at(indexAccessor.bufferView));
//
//			glDrawElements(primitive.mode, indexAccessor.count,
//				indexAccessor.componentType,
//				BUFFER_OFFSET(indexAccessor.byteOffset));
//		}
//	}
//
//	void GLTFStaticMesh::dbgModel(tinygltf::Model& model) {
//		for (auto& mesh : model.meshes) {
//			XENGINE_TRACE("mesh : {}", mesh.name);
//			for (auto& primitive : mesh.primitives) {
//				const tinygltf::Accessor& indexAccessor =
//					model.accessors[primitive.indices];
//
//				XENGINE_TRACE("indexaccessor: count {}, type "
//					, indexAccessor.count, indexAccessor.componentType);
//
//				tinygltf::Material& mat = model.materials[primitive.material];
//				for (auto& mats : mat.values) {
//					XENGINE_TRACE("mat : {}", mats.first.c_str());
//				}
//
//				for (auto& image : model.images) {
//					XENGINE_TRACE("image name : {}", image.uri);
//					XENGINE_TRACE("  size :  {}", image.image.size());
//					XENGINE_TRACE("  w/h : {}/{}", image.width, image.height);
//				}
//
//				XENGINE_TRACE("indices : {}", primitive.indices);
//				XENGINE_TRACE("mode    : ({})", primitive.mode);
//
//				for (auto& attrib : primitive.attributes) {
//					XENGINE_TRACE("attribute : {}", attrib.first.c_str());
//				}
//			}
//		}
//	}
//
//
//
//
//
//	/*GLTFStaticMesh::~GLTFStaticMesh()
//	{
//		glDeleteVertexArrays(1, &VAO_and_EBOs.first);
//
//		for (auto item = VAO_and_EBOs.second.cbegin(); item != VAO_and_EBOs.second.cend();)
//		{
//			glDeleteBuffers(1, &VAO_and_EBOs.second[item->first]);
//			VAO_and_EBOs.second.erase(item++);
//		}
//	}*/
//
//	//std::pair<unsigned int, std::map<int, unsigned int>> GLTFStaticMesh::bindModel()
//	//{
//	//	std::map<int, unsigned int> mEbos;
//	//	uint32_t mVao;
//	//	glGenVertexArrays(1, &mVao);
//	//	glBindVertexArray(mVao);
//
//	//	const tinygltf::Scene& scene = model.scenes[model.defaultScene];
//	//	for (size_t i = 0; i < scene.nodes.size(); ++i)
//	//	{
//	//		assert((scene.nodes[i] >= 0) && (scene.nodes[i] < model.nodes.size()));
//	//		bindModelNodes(mEbos, model.nodes[scene.nodes[i]]);
//	//	}
//	//	glBindVertexArray(0);
//
//	//	for (auto item = mEbos.cbegin(); item != mEbos.cend();)
//	//	{
//	//		tinygltf::BufferView bufferView = model.bufferViews[item->first];
//	//		if (bufferView.target != GL_ELEMENT_ARRAY_BUFFER)
//	//		{
//	//			glDeleteBuffers(1, &mEbos[item->first]);
//	//			mEbos.erase(item++);
//	//		}
//	//		else
//	//		{
//	//			++item;
//	//		}
//	//	}
//
//	//	return { mVao, mEbos };
//	//}
//
//	//void GLTFStaticMesh::bindModelNodes(std::map<int, unsigned int>& mEbos, tinygltf::Node& node)
//	//{
//	//	if ((node.mesh >= 0) && (node.mesh < model.meshes.size()))
//	//	{
//	//		bindMesh(mEbos, model.meshes[node.mesh]);
//	//	}
//
//	//	for (size_t i = 0; i < node.children.size(); i++)
//	//	{
//	//		assert((node.children[i] >= 0) && (node.children[i] < model.nodes.size()));
//	//		bindModelNodes(mEbos, model.nodes[node.children[i]]);
//	//	}
//	//}
//
//	//void GLTFStaticMesh::bindMesh(std::map<int, unsigned int>& mEbos, tinygltf::Mesh& mesh)
//	//{
//	//	for (int i = 0; i < model.bufferViews.size(); ++i)
//	//	{
//	//		const tinygltf::BufferView& bufferView = model.bufferViews[i];
//	//		if (bufferView.target == 0)
//	//		{
//	//			continue;
//	//		}
//
//	//		const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
//
//	//		GLuint mEbo;
//	//		glGenBuffers(1, &mEbo);
//	//		mEbos[i] = mEbo;
//	//		glBufferData(
//	//			bufferView.target, bufferView.byteLength,
//	//			&buffer.data.at(0) + bufferView.byteOffset, GL_STATIC_DRAW
//	//		);
//
//	//		for (size_t i = 0; i < mesh.primitives.size(); ++i)
//	//		{
//	//			tinygltf::Primitive primitive = mesh.primitives[i];
//	//			tinygltf::Accessor indexAccessor = model.accessors[primitive.indices];
//
//	//			for (auto& attrib : primitive.attributes)
//	//			{
//	//				tinygltf::Accessor accessor = model.accessors[attrib.second];
//	//				int byteStride = accessor.ByteStride(model.bufferViews[accessor.bufferView]);
//	//				glBindBuffer(GL_ARRAY_BUFFER, mEbos[accessor.bufferView]);
//
//	//				int size = 1;
//	//				if (accessor.type != TINYGLTF_TYPE_SCALAR)
//	//				{
//	//					size = accessor.type;
//	//				}
//
//	//				int attribute = -1;
//	//				if (attrib.first.compare("POSITION") == 0) { attribute = 0; }
//	//				if (attrib.first.compare("TEXCOORD_0") == 0) { attribute = 1; }
//	//				if (attrib.first.compare("NORMAL") == 0) { attribute = 2; }
//	//				if (attribute >= 0)
//	//				{
//	//					glEnableVertexAttribArray(attribute);
//	//					glVertexAttribPointer(
//	//						attribute, size, accessor.componentType, accessor.normalized ? GL_TRUE : GL_FALSE,
//	//						byteStride, (char*)NULL + accessor.byteOffset
//	//					);
//	//				}
//
//	//			}
//	//		}
//	//	}
//	//}
//
//	/*void GLTFStaticMesh::prepareForDrawing()
//	{ 
//		glBindVertexArray(VAO_and_EBOs.first);
//	}
//
//	void GLTFStaticMesh::draw() 
//	{
//		const tinygltf::Scene& scene = model.scenes[model.defaultScene];
//		for (size_t i = 0; i < scene.nodes.size(); ++i)
//		{
//			drawModelNodes(model.nodes[scene.nodes[i]]);
//		}
//	}
//
//	void GLTFStaticMesh::drawModelNodes(tinygltf::Node& node)
//	{
//		if ((node.mesh >= 0) && (node.mesh < model.meshes.size()))
//		{
//			drawMesh(VAO_and_EBOs.second, model.meshes[node.mesh]);
//		}
//		for (size_t i = 0; i < node.children.size(); ++i)
//		{
//			drawModelNodes(model.nodes[node.children[i]]);
//		}
//	}
//
//	void GLTFStaticMesh::drawMesh(const std::map<int, unsigned int>& mEbos, tinygltf::Mesh& mesh) 
//	{
//		for (int i = 0; i < mesh.primitives.size(); ++i)
//		{
//			tinygltf::Primitive& primitive = mesh.primitives[i];
//			tinygltf::Accessor& indexAccessor = model.accessors[primitive.indices];
//
//			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mEbos.at(indexAccessor.bufferView));
//			glDrawElements(
//				primitive.mode, indexAccessor.count,
//				indexAccessor.componentType,
//				(char*)NULL + indexAccessor.byteOffset
//			);
//		}
//	}*/
//}