#include "graphics/gltfLoader.h"

#define TINYGLTF_IMPLEMENTATION
//#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
//#define STBI_MSC_SECURE_CRT
#define TINYGLTF_NOEXCEPTION
#define JSON_NOEXCEPTION
#include "external/tinygltf/tiny_gltf.h"
#include "log.h"
#include "graphics/shader.h"
#include "glad/glad.h"
#include "external/glm/gtc/type_ptr.hpp"
#include <iostream>

#define BUFFER_OFFSET(i) ((char *)NULL + (i))


namespace XEngine::graphics
{

	glm::mat4 GetLocalMatrix(const tinygltf::Node& node) {
		glm::mat4 mat = glm::mat4(1.0f);
		if (!node.matrix.empty()) {
			mat = glm::make_mat4(node.matrix.data());
		}
		else {
			if (!node.translation.empty()) {
				mat = glm::translate(mat, glm::vec3(node.translation[0], node.translation[1], node.translation[2]));
			}
			if (!node.rotation.empty()) {
				glm::quat q(static_cast<float>(node.rotation[3]), static_cast<float>(node.rotation[0]), static_cast<float>(node.rotation[1]), static_cast<float>(node.rotation[2]));
				mat *= glm::mat4_cast(q);
			}
			if (!node.scale.empty()) {
				mat = glm::scale(mat, glm::vec3(node.scale[0], node.scale[1], node.scale[2]));
			}
		}
		return mat;
	}



	// tinygltf::Model model;

	GLTFStaticMesh::GLTFStaticMesh(tinygltf::Model& model, const char* filename)
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

		vaoAndEbos = bindModel(model);
		extractTriangles(model);

	}

	GLTFStaticMesh::~GLTFStaticMesh()
	{
		glDeleteVertexArrays(1, &vaoAndEbos.first);

		// 遍歷 map 中所有的 buffer 並刪除
		for (auto const& [key, val] : vaoAndEbos.second)
		{
			glDeleteBuffers(1, &val);
		}
		vaoAndEbos.second.clear(); // 清空 map
	}

	void GLTFStaticMesh::bindMesh(std::map<int, GLuint>& vbos,
		tinygltf::Model& model, tinygltf::Mesh& mesh) {
		for (int i = 0; i < model.bufferViews.size(); ++i) {
			const tinygltf::BufferView& bufferView = model.bufferViews[i];
			if (bufferView.target == 0) {  // TODO impl drawarrays
				XENGINE_WARN("WARN: bufferView.target is zero");
				continue;  // Unsupported bufferView.
				/*
				  From spec2.0 readme:
				  https://github.com/KhronosGroup/glTF/tree/master/specification/2.0
						   ... drawArrays function should be used with a count equal to
				  the count            property of any of the accessors referenced by the
				  attributes            property            (they are all equal for a given
				  primitive).
				*/
			}

			const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
			//XENGINE_TRACE("bufferview.target {} ", bufferView.target);

			GLuint vbo;
			glGenBuffers(1, &vbo);
			vbos[i] = vbo;
			glBindBuffer(bufferView.target, vbo);

			/*XENGINE_TRACE("buffer.data.size = {}, bufferview.byteOffset = {}"
				, buffer.data.size(), bufferView.byteOffset);*/

			glBufferData(bufferView.target, bufferView.byteLength,
				&buffer.data.at(0) + bufferView.byteOffset, GL_STATIC_DRAW);
		}

		for (size_t i = 0; i < mesh.primitives.size(); ++i) {
			tinygltf::Primitive primitive = mesh.primitives[i];
			tinygltf::Accessor indexAccessor = model.accessors[primitive.indices];

			for (auto& attrib : primitive.attributes) {
				tinygltf::Accessor accessor = model.accessors[attrib.second];
				int byteStride =
					accessor.ByteStride(model.bufferViews[accessor.bufferView]);
				glBindBuffer(GL_ARRAY_BUFFER, vbos[accessor.bufferView]);

				int size = 1;
				if (accessor.type != TINYGLTF_TYPE_SCALAR) {
					size = accessor.type;
				}

				int vaa = -1;
				if (attrib.first.compare("POSITION") == 0) vaa = 0;
				if (attrib.first.compare("NORMAL") == 0) vaa = 1;
				if (attrib.first.compare("TEXCOORD_0") == 0) vaa = 2;
				if (vaa > -1) {
					glEnableVertexAttribArray(vaa);
					glVertexAttribPointer(vaa, size, accessor.componentType,
						accessor.normalized ? GL_TRUE : GL_FALSE,
						byteStride, BUFFER_OFFSET(accessor.byteOffset));
				}
				else
					XENGINE_WARN("vaa missing: {}", attrib.first);
			}

			if (model.textures.size() > 0) {
				// fixme: Use material's baseColor
				tinygltf::Texture& tex = model.textures[0];

				if (tex.source > -1) {

					GLuint texid;
					glGenTextures(1, &texid);

					tinygltf::Image& image = model.images[tex.source];

					glBindTexture(GL_TEXTURE_2D, texid);
					glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
					glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
					glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

					GLenum format = GL_RGBA;

					if (image.component == 1) {
						format = GL_RED;
					}
					else if (image.component == 2) {
						format = GL_RG;
					}
					else if (image.component == 3) {
						format = GL_RGB;
					}
					else {
						// ???
					}

					GLenum type = GL_UNSIGNED_BYTE;
					if (image.bits == 8) {
						// ok
					}
					else if (image.bits == 16) {
						type = GL_UNSIGNED_SHORT;
					}
					else {
						// ???
					}

					glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width, image.height, 0,
						format, type, &image.image.at(0));
				}
			}
		}
	}

	// bind models
	void GLTFStaticMesh::bindModelNodes(std::map<int, GLuint>& vbos, tinygltf::Model& model,
		tinygltf::Node& node) {
		if ((node.mesh >= 0) && (node.mesh < model.meshes.size())) {
			bindMesh(vbos, model, model.meshes[node.mesh]);
		}

		for (size_t i = 0; i < node.children.size(); i++) {
			assert((node.children[i] >= 0) && (node.children[i] < model.nodes.size()));
			bindModelNodes(vbos, model, model.nodes[node.children[i]]);
		}
	}

	std::pair<GLuint, std::map<int, GLuint>> GLTFStaticMesh::bindModel(tinygltf::Model& model) {
		std::map<int, GLuint> vbos;
		GLuint vao;
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		const tinygltf::Scene& scene = model.scenes[model.defaultScene];
		for (size_t i = 0; i < scene.nodes.size(); ++i) {
			assert((scene.nodes[i] >= 0) && (scene.nodes[i] < model.nodes.size()));
			bindModelNodes(vbos, model, model.nodes[scene.nodes[i]]);
		}

		glBindVertexArray(0);
		// cleanup vbos but do not delete index buffers yet
		/*for (auto it = vbos.cbegin(); it != vbos.cend();) {
			tinygltf::BufferView bufferView = model.bufferViews[it->first];
			if (bufferView.target != GL_ELEMENT_ARRAY_BUFFER) {
				glDeleteBuffers(1, &vbos[it->first]);
				vbos.erase(it++);
			}
			else {
				++it;
			}
		}*/

		return { vao, vbos };
	}

	// recursively draw node and children nodes of model
	void GLTFStaticMesh::drawModelNodes(const std::pair<GLuint, std::map<int, GLuint>>& vaoAndEbos,
		tinygltf::Model& model, tinygltf::Node& node) {
		if ((node.mesh >= 0) && (node.mesh < model.meshes.size())) {
			drawMesh(vaoAndEbos.second, model, model.meshes[node.mesh]);
		}
		for (size_t i = 0; i < node.children.size(); i++) {
			drawModelNodes(vaoAndEbos, model, model.nodes[node.children[i]]);
		}
	}


	void GLTFStaticMesh::drawMesh(const std::map<int, GLuint>& vbos,
		tinygltf::Model& model, tinygltf::Mesh& mesh) {
		for (size_t i = 0; i < mesh.primitives.size(); ++i) {
			tinygltf::Primitive primitive = mesh.primitives[i];
			tinygltf::Accessor indexAccessor = model.accessors[primitive.indices];

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbos.at(indexAccessor.bufferView));

			glDrawElements(primitive.mode, (GLsizei)indexAccessor.count,
				indexAccessor.componentType,
				BUFFER_OFFSET(indexAccessor.byteOffset));
		}
	}

	void GLTFStaticMesh::dbgModel(tinygltf::Model& model) {
		for (auto& mesh : model.meshes) {
			XENGINE_TRACE("mesh : {}", mesh.name);
			for (auto& primitive : mesh.primitives) {
				const tinygltf::Accessor& indexAccessor =
					model.accessors[primitive.indices];

				XENGINE_TRACE("indexaccessor: count {}, type : {}"
					, indexAccessor.count, indexAccessor.componentType);


				tinygltf::Material& mat = model.materials[primitive.material];
				for (auto& mats : mat.values) {
					XENGINE_TRACE("mat : {}", mats.first.c_str());
				}

				for (auto& image : model.images) {
					XENGINE_TRACE("image name : {}", image.uri);
					XENGINE_TRACE("  size :  {}", image.image.size());
					XENGINE_TRACE("  w/h : {}/{}", image.width, image.height);
				}

				XENGINE_TRACE("indices : {}", primitive.indices);
				XENGINE_TRACE("mode    : ({})", primitive.mode);

				for (auto& attrib : primitive.attributes) {
					XENGINE_TRACE("attribute : {}", attrib.first.c_str());
				}

			}
		}



		/*for (auto& buffers : model.buffers)
		{
			XENGINE_TRACE("buffers : {}", buffers.data.data());
		}*/
	}

	void GLTFStaticMesh::drawModel(tinygltf::Model& model) {
		glBindVertexArray(vaoAndEbos.first);

		const tinygltf::Scene& scene = model.scenes[model.defaultScene];
		for (size_t i = 0; i < scene.nodes.size(); ++i) {
			drawModelNodes(vaoAndEbos, model, model.nodes[scene.nodes[i]]);
		}

		glBindVertexArray(0);
	}

	void GLTFStaticMesh::extractTriangles(tinygltf::Model& model)
	{
		mTriangles.clear();
		const tinygltf::Scene& scene = model.scenes[model.defaultScene > -1 ? model.defaultScene : 0];

		// 從根節點開始，初始變換是單位矩陣
		for (size_t i = 0; i < scene.nodes.size(); ++i) {
			const tinygltf::Node& node = model.nodes[scene.nodes[i]];
			extractNodeTriangles(model, node, glm::mat4(1.0f));
		}
	}


	void GLTFStaticMesh::extractNodeTriangles(tinygltf::Model& model, const tinygltf::Node& node, const glm::mat4& parentTransform)
	{
		glm::mat4 worldTransform = parentTransform * GetLocalMatrix(node);
		glm::mat3 normalTransform = glm::transpose(glm::inverse(glm::mat3(worldTransform)));

		if (node.mesh > -1) {
			const tinygltf::Mesh& mesh = model.meshes[node.mesh];
			for (size_t p = 0; p < mesh.primitives.size(); ++p) {
				const tinygltf::Primitive& primitive = mesh.primitives[p];
				if (primitive.indices < 0 || primitive.attributes.find("POSITION") == primitive.attributes.end()) {
					continue; // 跳過沒有索引或沒有頂點位置的 primitive
				}

				// --- 顶点位置数据 ---
				const tinygltf::Accessor& indexAccessor = model.accessors[primitive.indices];
				const tinygltf::BufferView& indexBufferView = model.bufferViews[indexAccessor.bufferView];
				const tinygltf::Buffer& indexBuffer = model.buffers[indexBufferView.buffer];
				// 指標計算：同時考慮 bufferView 和 accessor 的 byteOffset
				const uint8_t* indexBufferData = &indexBuffer.data[indexBufferView.byteOffset + indexAccessor.byteOffset];

				const tinygltf::Accessor& posAccessor = model.accessors.at(primitive.attributes.at("POSITION"));
				const tinygltf::BufferView& posBufferView = model.bufferViews[posAccessor.bufferView];
				const tinygltf::Buffer& posBuffer = model.buffers[posBufferView.buffer];
				const uint8_t* posBufferStart = &posBuffer.data[posBufferView.byteOffset + posAccessor.byteOffset];
				// 使用 byteStride 來正確地跳轉到下一個頂點
				size_t posByteStride = posAccessor.ByteStride(posBufferView);

				// --- 法线数据 ---
				const uint8_t* normalBufferStart = nullptr;
				size_t normalByteStride = 0;
				const tinygltf::Accessor* normalAccessor = nullptr; // 使用指標以處理不存在的情況
				if (primitive.attributes.count("NORMAL")) {
					normalAccessor = &model.accessors.at(primitive.attributes.at("NORMAL"));
					const tinygltf::BufferView& normalBufferView = model.bufferViews[normalAccessor->bufferView];
					const tinygltf::Buffer& normalBuffer = model.buffers[normalBufferView.buffer];
					normalBufferStart = &normalBuffer.data[normalBufferView.byteOffset + normalAccessor->byteOffset];
					normalByteStride = normalAccessor->ByteStride(normalBufferView);
				}

				// --- 獲取頂點總數以進行邊界檢查 ---
				const size_t vertexCount = posAccessor.count;

				for (size_t j = 0; j < indexAccessor.count; j += 3) 
				{
					// 1. 在迴圈開始時進行初始化
					unsigned int i0 = 0, i1 = 0, i2 = 0;
					bool indices_valid = true; // 添加一個標誌來追蹤索引是否成功讀取

					// 2. 讀取索引，並在不支援的類型時設置標誌
					if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) 
					{
						const uint16_t* indices = reinterpret_cast<const uint16_t*>(&indexBufferData[j * sizeof(uint16_t)]);
						i0 = indices[0];
						i1 = indices[1];
						i2 = indices[2];
					}
					else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) 
					{
						const uint32_t* indices = reinterpret_cast<const uint32_t*>(&indexBufferData[j * sizeof(uint32_t)]);
						i0 = indices[0];
						i1 = indices[1];
						i2 = indices[2];
					}
					else 
					{
						// 如果索引類型不支援，設置標誌並準備跳過
						XENGINE_WARN("Unsupported index component type: {}", indexAccessor.componentType);
						indices_valid = false;
					}

					// 3. 檢查標誌，如果無效則跳過此三角形
					if (!indices_valid) 
					{
						continue;
					}

					// --- 新增：邊界檢查 (現在 i0, i1, i2 肯定是初始化的) ---
					if (i0 >= vertexCount || i1 >= vertexCount || i2 >= vertexCount)
					{
						XENGINE_WARN("Vertex index out of bounds! Index values: ({}, {}, {}), Vertex count: {}. Skipping triangle.", i0, i1, i2, vertexCount);
						continue; // 跳過這個無效的三角形
					}
					// --- 更安全地讀取頂點和法線 ---
					// 使用 byte-addressing，而不是假設它們是緊密排列的 float 陣列
					const float* v0_ptr = reinterpret_cast<const float*>(posBufferStart + i0 * posByteStride);
					const float* v1_ptr = reinterpret_cast<const float*>(posBufferStart + i1 * posByteStride);
					const float* v2_ptr = reinterpret_cast<const float*>(posBufferStart + i2 * posByteStride);

					glm::vec3 v0_local(v0_ptr[0], v0_ptr[1], v0_ptr[2]);
					glm::vec3 v1_local(v1_ptr[0], v1_ptr[1], v1_ptr[2]);
					glm::vec3 v2_local(v2_ptr[0], v2_ptr[1], v2_ptr[2]);

					glm::vec3 n0_local(0.0f, 1.0f, 0.0f), n1_local(0.0f, 1.0f, 0.0f), n2_local(0.0f, 1.0f, 0.0f);
					if (normalBufferStart && normalAccessor && i0 < normalAccessor->count && i1 < normalAccessor->count && i2 < normalAccessor->count) {
						const float* n0_ptr = reinterpret_cast<const float*>(normalBufferStart + i0 * normalByteStride);
						const float* n1_ptr = reinterpret_cast<const float*>(normalBufferStart + i1 * normalByteStride);
						const float* n2_ptr = reinterpret_cast<const float*>(normalBufferStart + i2 * normalByteStride);
						n0_local = glm::vec3(n0_ptr[0], n0_ptr[1], n0_ptr[2]);
						n1_local = glm::vec3(n1_ptr[0], n1_ptr[1], n1_ptr[2]);
						n2_local = glm::vec3(n2_ptr[0], n2_ptr[1], n2_ptr[2]);
					}

					// --- 变换到世界空间 ---
					glm::vec3 v0_world = worldTransform * glm::vec4(v0_local, 1.0f);
					glm::vec3 v1_world = worldTransform * glm::vec4(v1_local, 1.0f);
					glm::vec3 v2_world = worldTransform * glm::vec4(v2_local, 1.0f);

					glm::vec3 n0_world = glm::normalize(normalTransform * n0_local);
					glm::vec3 n1_world = glm::normalize(normalTransform * n1_local);
					glm::vec3 n2_world = glm::normalize(normalTransform * n2_local);

					// --- 更新包围盒 ---
					m_boundsMin = glm::min(m_boundsMin, v0_world);
					m_boundsMin = glm::min(m_boundsMin, v1_world);
					m_boundsMin = glm::min(m_boundsMin, v2_world);
					m_boundsMax = glm::max(m_boundsMax, v0_world);
					m_boundsMax = glm::max(m_boundsMax, v1_world);
					m_boundsMax = glm::max(m_boundsMax, v2_world);

					// --- 填充并存入 mTriangles 向量 ---
					Triangle tri;
					tri.v1 = v0_local;
					tri.v2 = v1_local;
					tri.v3 = v2_local;
					tri.NA = n0_local;
					tri.NB = n1_local;
					tri.NC = n2_local;
					tri.faceNormal = glm::normalize(glm::cross(v1_local - v0_local, v2_local - v0_local));
					// padding 成员不需要手动赋值，它們的存在只是为了占位

					mTriangles.push_back(tri);
				}
			}
		}

	/*	XENGINE_TRACE("=========================================");
		XENGINE_TRACE("CPU-Side Triangle Data Verification:");
		XENGINE_TRACE("Total triangles extracted: {}", mTriangles.size());
		if (mTriangles.size() > 0) {
			for (int i = 0; i < 100; i++)
			{
				XENGINE_TRACE("--- The {}-th Triangle ---", i);
				XENGINE_TRACE("v1: ({:.2f}, {:.2f}, {:.2f})", mTriangles[i].v1.x, mTriangles[i].v1.y, mTriangles[i].v1.z);
				XENGINE_TRACE("v2: ({:.2f}, {:.2f}, {:.2f})", mTriangles[i].v2.x, mTriangles[i].v2.y, mTriangles[i].v2.z);
				XENGINE_TRACE("v3: ({:.2f}, {:.2f}, {:.2f})", mTriangles[i].v3.x, mTriangles[i].v3.y, mTriangles[i].v3.z);
				XENGINE_INFO("--- The {}-th Triangle of Normal ---", i);
				XENGINE_INFO("v1: ({:.2f}, {:.2f}, {:.2f})", mTriangles[i].NA.x, mTriangles[i].NA.y, mTriangles[i].NA.z);
				XENGINE_INFO("v2: ({:.2f}, {:.2f}, {:.2f})", mTriangles[i].NB.x, mTriangles[i].NB.y, mTriangles[i].NB.z);
				XENGINE_INFO("v3: ({:.2f}, {:.2f}, {:.2f})", mTriangles[i].NC.x, mTriangles[i].NC.y, mTriangles[i].NC.z);
			}
		}		
		XENGINE_TRACE("=========================================");*/

		// 遞迴處理子節點
		for (size_t i = 0; i < node.children.size(); ++i) {
			extractNodeTriangles(model, model.nodes[node.children[i]], worldTransform);
		}
	}


}


