#include "graphics/gltfLoader.h"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_MSC_SECURE_CRT
#define TINYGLTF_NOEXCEPTION
#define JSON_NOEXCEPTION
#include "external/tinygltf/tiny_gltf.h"
#include "log.h"
#include "shaders/shader.h"
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

		loadTextures(model);
		loadMaterials(model);
		vaoAndEbos = bindModel(model);
		// extractTriangles(model);
		extractMesh(model);

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

		if (!mTextures.empty()) {
			glDeleteTextures((GLsizei)mTextures.size(), mTextures.data());
			mTextures.clear();
		}
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

	void GLTFStaticMesh::loadTextures(tinygltf::Model& model)
	{
		mTextures.resize(model.textures.size());
		if (model.textures.empty()) return;
		glGenTextures((GLsizei)model.textures.size(), mTextures.data());

		for (size_t i = 0; i < model.textures.size(); i++) {
			const tinygltf::Texture& tex = model.textures[i];

			if (tex.source < 0) {
				continue;
			}

			glBindTexture(GL_TEXTURE_2D, mTextures[i]);

			const tinygltf::Image& image = model.images[tex.source];

			GLenum format = GL_RGBA;
			if (image.component == 1) format = GL_RED;
			else if (image.component == 2) format = GL_RG;
			else if (image.component == 3) format = GL_RGB;

			GLenum type = GL_UNSIGNED_BYTE;
			if (image.bits == 16) type = GL_UNSIGNED_SHORT;

			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width, image.height, 0,
				format, type, &image.image.at(0));

			// 設置紋理參數 (Sampler)
			if (tex.sampler >= 0) {
				const tinygltf::Sampler& sampler = model.samplers[tex.sampler];
				// glTF enums directly map to OpenGL enums
				// https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#appendix-a-enums
				// 如果 glTF 檔案中沒有定義，tinygltf 會給出預設值 (通常是 REPEAT 和 LINEAR)
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, sampler.minFilter == -1 ? GL_LINEAR_MIPMAP_LINEAR : sampler.minFilter);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, sampler.magFilter == -1 ? GL_LINEAR : sampler.magFilter);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, sampler.wrapS);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, sampler.wrapT);
			}
			else {
				// 如果 glTF 紋理沒有指定 sampler，使用預設值
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			}

			glGenerateMipmap(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, 0);
		}
	}

	void GLTFStaticMesh::loadMaterials(tinygltf::Model& model)
	{
		mMaterials.clear();
		for (const auto& mat : model.materials) {
			Material newMaterial;

			// 檢查 KHR_materials_unlit 擴展
			if (mat.extensions.find("KHR_materials_unlit") != mat.extensions.end()) {
				newMaterial.isUnlit = true;
			}

			// 讀取 PBR Metallic-Roughness 屬性
			const auto& pbr = mat.pbrMetallicRoughness;

			// Base Color
			if (pbr.baseColorFactor.size() == 4) {
				newMaterial.baseColorFactor = glm::make_vec4(pbr.baseColorFactor.data());
			}
			newMaterial.baseColorTexture = pbr.baseColorTexture.index;

			// Metallic and Roughness
			newMaterial.metallicFactor = (float)pbr.metallicFactor;
			newMaterial.roughnessFactor = (float)pbr.roughnessFactor;
			newMaterial.metallicRoughnessTexture = pbr.metallicRoughnessTexture.index;

			// Normal Map
			newMaterial.normalTexture = mat.normalTexture.index;

			// Emissive Map
			if (mat.emissiveFactor.size() == 3) {
				newMaterial.emissiveFactor = glm::make_vec3(mat.emissiveFactor.data());
			}
			newMaterial.emissiveTexture = mat.emissiveTexture.index;

			// (可選) Occlusion Map
			// newMaterial.occlusionTexture = mat.occlusionTexture.index;

			mMaterials.push_back(newMaterial);
		}
		XENGINE_TRACE("Loaded {} materials from glTF file.", mMaterials.size());
	}


	void GLTFStaticMesh::drawMesh(const std::map<int, GLuint>& vbos,
		tinygltf::Model& model, tinygltf::Mesh& mesh) {
		for (size_t i = 0; i < mesh.primitives.size(); ++i) {
			tinygltf::Primitive primitive = mesh.primitives[i];
			tinygltf::Accessor indexAccessor = model.accessors[primitive.indices];

			// <-- 新增紋理綁定邏輯
			if (primitive.material >= 0) {
				const tinygltf::Material& material = model.materials[primitive.material];

				if (material.pbrMetallicRoughness.baseColorTexture.index >= 0) {
					// 激活紋理單元 0
					glActiveTexture(GL_TEXTURE0);
					// 獲取紋理索引並綁定
					int tex_idx = material.pbrMetallicRoughness.baseColorTexture.index;
					glBindTexture(GL_TEXTURE_2D, mTextures[tex_idx]);
					// 假設你的 shader 中的 sampler uniform 已經設置為 0
					// (例如: shader->setInt("texture_diffuse1", 0);)
				}
			}


			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbos.at(indexAccessor.bufferView));

			glDrawElements(primitive.mode, (GLsizei)indexAccessor.count,
				indexAccessor.componentType,
				BUFFER_OFFSET(indexAccessor.byteOffset));

			// (可選) 解除綁定，避免狀態洩漏
			glBindTexture(GL_TEXTURE_2D, 0);
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

	// 替換你的 extractMesh 函數
	void GLTFStaticMesh::extractMesh(tinygltf::Model& model)
	{
		// 清空所有 Mesh 數據
		mMesh.vertices.clear();
		mMesh.normals.clear();
		mMesh.indices.clear();
		mMesh.texCoords.clear();
		mMesh.materialIndices.clear();

		// 新增一個變數來追蹤所有已處理頂點的總數，作為索引的基底偏移量
		unsigned int vertex_offset = 0;

		const tinygltf::Scene& scene = model.scenes[model.defaultScene > -1 ? model.defaultScene : 0];

		// 從根節點開始，遞歸遍歷場景圖
		for (size_t i = 0; i < scene.nodes.size(); ++i) 
		{
			const tinygltf::Node& node = model.nodes[scene.nodes[i]];
			// 將 vertex_offset 作為引用傳遞，以便在遞歸中累加
			extractNodeMesh(model, node, glm::mat4(1.0f), vertex_offset);
		}
	}

// 替換你的 extractNodeMesh 函數
	void GLTFStaticMesh::extractNodeMesh(tinygltf::Model& model, const tinygltf::Node& node, const glm::mat4& parentTransform, unsigned int& vertex_offset)
	{
		// 計算當前節點的世界變換矩陣
		glm::mat4 worldTransform = parentTransform * GetLocalMatrix(node);
		glm::mat3 normalTransform = glm::transpose(glm::inverse(glm::mat3(worldTransform)));
	
		// 如果節點包含一個 mesh，則處理它
		if (node.mesh > -1) {
			const tinygltf::Mesh& mesh = model.meshes[node.mesh];
			// 遍歷 mesh 中的所有 primitive (每個 primitive 都是一個獨立的繪製調用)
			for (size_t p = 0; p < mesh.primitives.size(); ++p) {
				const tinygltf::Primitive& primitive = mesh.primitives[p];
				if (primitive.indices < 0 || primitive.attributes.find("POSITION") == primitive.attributes.end()) {
					continue; // 跳過沒有索引或沒有頂點位置的 primitive
				}
	
				// --- 獲取數據訪問器 (Accessor) ---
				const tinygltf::Accessor& indexAccessor = model.accessors[primitive.indices];
				const tinygltf::Accessor& posAccessor = model.accessors.at(primitive.attributes.at("POSITION"));
				const tinygltf::Accessor* normalAccessor = nullptr;
				if (primitive.attributes.count("NORMAL")) {
					normalAccessor = &model.accessors.at(primitive.attributes.at("NORMAL"));
				}
				const tinygltf::Accessor* uvAccessor = nullptr;
				if (primitive.attributes.count("TEXCOORD_0")) {
					uvAccessor = &model.accessors.at(primitive.attributes.at("TEXCOORD_0"));
				}

				
	
				// --- 將這個 primitive 的所有頂點數據追加到我們的 Mesh 結構中 ---
				// 獲取頂點位置數據的指針和步長
				const tinygltf::BufferView& posBufferView = model.bufferViews[posAccessor.bufferView];
				const tinygltf::Buffer& posBuffer = model.buffers[posBufferView.buffer];
				const uint8_t* posBufferStart = &posBuffer.data[posBufferView.byteOffset + posAccessor.byteOffset];
				size_t posByteStride = posAccessor.ByteStride(posBufferView);
	
				// 獲取法線數據的指針和步長 (如果存在)
				const uint8_t* normalBufferStart = nullptr;
				size_t normalByteStride = 0;
				if (normalAccessor) {
					const tinygltf::BufferView& normalBufferView = model.bufferViews[normalAccessor->bufferView];
					normalBufferStart = &model.buffers[normalBufferView.buffer].data[normalBufferView.byteOffset + normalAccessor->byteOffset];
					normalByteStride = normalAccessor->ByteStride(normalBufferView);
				}
	
				// 獲取 UV 數據的指針和步長 (如果存在)
				const uint8_t* uvBufferStart = nullptr;
				size_t uvByteStride = 0;
				if (uvAccessor) {
					const tinygltf::BufferView& uvBufferView = model.bufferViews[uvAccessor->bufferView];
					uvBufferStart = &model.buffers[uvBufferView.buffer].data[uvBufferView.byteOffset + uvAccessor->byteOffset];
					uvByteStride = uvAccessor->ByteStride(uvBufferView);
				}
	
				// 遍歷這個 primitive 的所有頂點，並追加到 mMesh
				for (size_t v_idx = 0; v_idx < posAccessor.count; ++v_idx) {
					// 讀取局部頂點位置
					const float* v_ptr = reinterpret_cast<const float*>(posBufferStart + v_idx * posByteStride);
					glm::vec3 v_local(v_ptr[0], v_ptr[1], v_ptr[2]);
	
					// 變換到世界空間
					glm::vec3 v_world = worldTransform * glm::vec4(v_local, 1.0f);
					mMesh.vertices.push_back(glm::vec4(v_world, 1.0f));
	
					// 更新包圍盒
					m_boundsMin = glm::min(m_boundsMin, v_world);
					m_boundsMax = glm::max(m_boundsMax, v_world);
	
					// 讀取並追加 UV (如果存在)
					if (uvAccessor && uvBufferStart) {
						const float* uv_ptr = reinterpret_cast<const float*>(uvBufferStart + v_idx * uvByteStride);
						mMesh.texCoords.push_back(glm::vec2(uv_ptr[0], uv_ptr[1]));
					}
					else 
					{
						mMesh.texCoords.push_back(glm::vec2(0.0f)); // 如果沒有 UV，填充默認值
					}
				}

				// --- 處理索引數據，並應用全局偏移量 ---
				const tinygltf::BufferView& indexBufferView = model.bufferViews[indexAccessor.bufferView];
				const uint8_t* indexBufferData = &model.buffers[indexBufferView.buffer].data[indexBufferView.byteOffset + indexAccessor.byteOffset];
				int materialIndex = primitive.material;
	
				// 遍歷這個 primitive 的所有索引
				for (size_t j = 0; j < indexAccessor.count; j += 3) {
					unsigned int i0 = 0, i1 = 0, i2 = 0;
	
					// 根據 componentType 讀取局部的索引值
					if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
						const uint16_t* indices = reinterpret_cast<const uint16_t*>(&indexBufferData[j * sizeof(uint16_t)]);
						i0 = indices[0]; i1 = indices[1]; i2 = indices[2];
					}
					else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
						const uint32_t* indices = reinterpret_cast<const uint32_t*>(&indexBufferData[j * sizeof(uint32_t)]);
						i0 = indices[0]; i1 = indices[1]; i2 = indices[2];
					}
					else {
						XENGINE_WARN("Unsupported index component type: {}", indexAccessor.componentType);
						continue;
					}
	
					// 【核心修改】將局部索引加上基底偏移量，得到全局索引
					glm::ivec4 global_indices(
						i0 + vertex_offset,
						i1 + vertex_offset,
						i2 + vertex_offset,
						-1 // w 分量未使用
					);
					mMesh.indices.push_back(global_indices);

					glm::vec3 n0_local(0.0f, 1.0f, 0.0f), n1_local(0.0f, 1.0f, 0.0f), n2_local(0.0f, 1.0f, 0.0f);
					if (normalBufferStart && normalAccessor && i0 < normalAccessor->count && i1 < normalAccessor->count && i2 < normalAccessor->count) {
						const float* n0_ptr = reinterpret_cast<const float*>(normalBufferStart + i0 * normalByteStride);
						const float* n1_ptr = reinterpret_cast<const float*>(normalBufferStart + i1 * normalByteStride);
						const float* n2_ptr = reinterpret_cast<const float*>(normalBufferStart + i2 * normalByteStride);
						n0_local = glm::vec3(n0_ptr[0], n0_ptr[1], n0_ptr[2]);
						n1_local = glm::vec3(n1_ptr[0], n1_ptr[1], n1_ptr[2]);
						n2_local = glm::vec3(n2_ptr[0], n2_ptr[1], n2_ptr[2]);
					}

					glm::vec3 n0_world = glm::normalize(normalTransform * n0_local);
					glm::vec3 n1_world = glm::normalize(normalTransform * n1_local);
					glm::vec3 n2_world = glm::normalize(normalTransform * n2_local);

					mMesh.normals.push_back(glm::vec4(n0_world, 0.0f));
					mMesh.normals.push_back(glm::vec4(n1_world, 0.0f));
					mMesh.normals.push_back(glm::vec4(n2_world, 0.0f));
					
					
					
					// 為這個新生成的三角形記錄材質索引
					mMesh.materialIndices.push_back(materialIndex);
				}
	
				// 【核心修改】處理完一個 primitive 後，更新全局頂點偏移量
				vertex_offset += (unsigned int)posAccessor.count;
			}
		}
	
		// 遞歸處理子節點
		for (size_t i = 0; i < node.children.size(); ++i) 
		{
			extractNodeMesh(model, model.nodes[node.children[i]], worldTransform, vertex_offset);
		}
	
	}
}




	//void GLTFStaticMesh::extractMesh(tinygltf::Model& model)
	//{
	//	mMesh.vertices.clear();
	//	mMesh.normals.clear();
	//	mMesh.indices.clear();
	//	mMesh.texCoords.clear();
	//	const tinygltf::Scene& scene = model.scenes[model.defaultScene > -1 ? model.defaultScene : 0];

	//	// 從根節點開始，初始變換是單位矩陣
	//	for (size_t i = 0; i < scene.nodes.size(); ++i) {
	//		const tinygltf::Node& node = model.nodes[scene.nodes[i]];
	//		extractNodeMesh(model, node, glm::mat4(1.0f));
	//	}
	//}

	//void GLTFStaticMesh::extractNodeMesh(tinygltf::Model& model, const tinygltf::Node& node, const glm::mat4& parentTransform)
	//{
	//	glm::mat4 worldTransform = parentTransform * GetLocalMatrix(node);
	//	glm::mat3 normalTransform = glm::transpose(glm::inverse(glm::mat3(worldTransform)));

	//	if (node.mesh > -1) {
	//		const tinygltf::Mesh& mesh = model.meshes[node.mesh];
	//		for (size_t p = 0; p < mesh.primitives.size(); ++p) {
	//			const tinygltf::Primitive& primitive = mesh.primitives[p];
	//			if (primitive.indices < 0 || primitive.attributes.find("POSITION") == primitive.attributes.end()) {
	//				continue; // 跳過沒有索引或沒有頂點位置的 primitive
	//			}

	//			// 獲取當前 primitive 的材質索引
	//			int materialIndex = primitive.material;

	//			// --- 顶点位置数据 ---
	//			const tinygltf::Accessor& indexAccessor = model.accessors[primitive.indices];
	//			const tinygltf::BufferView& indexBufferView = model.bufferViews[indexAccessor.bufferView];
	//			const tinygltf::Buffer& indexBuffer = model.buffers[indexBufferView.buffer];
	//			// 指標計算：同時考慮 bufferView 和 accessor 的 byteOffset
	//			const uint8_t* indexBufferData = &indexBuffer.data[indexBufferView.byteOffset + indexAccessor.byteOffset];

	//			const tinygltf::Accessor& posAccessor = model.accessors.at(primitive.attributes.at("POSITION"));
	//			const tinygltf::BufferView& posBufferView = model.bufferViews[posAccessor.bufferView];
	//			const tinygltf::Buffer& posBuffer = model.buffers[posBufferView.buffer];
	//			const uint8_t* posBufferStart = &posBuffer.data[posBufferView.byteOffset + posAccessor.byteOffset];
	//			// 使用 byteStride 來正確地跳轉到下一個頂點
	//			size_t posByteStride = posAccessor.ByteStride(posBufferView);

	//			// --- 法线数据 ---
	//			const uint8_t* normalBufferStart = nullptr;
	//			size_t normalByteStride = 0;
	//			const tinygltf::Accessor* normalAccessor = nullptr; // 使用指標以處理不存在的情況
	//			if (primitive.attributes.count("NORMAL")) {
	//				normalAccessor = &model.accessors.at(primitive.attributes.at("NORMAL"));
	//				const tinygltf::BufferView& normalBufferView = model.bufferViews[normalAccessor->bufferView];
	//				const tinygltf::Buffer& normalBuffer = model.buffers[normalBufferView.buffer];
	//				normalBufferStart = &normalBuffer.data[normalBufferView.byteOffset + normalAccessor->byteOffset];
	//				normalByteStride = normalAccessor->ByteStride(normalBufferView);
	//			}

	//			// --- [新增] 紋理座標 (UV) 数据 ---
	//			const uint8_t* uvBufferStart = nullptr;
	//			size_t uvByteStride = 0;
	//			const tinygltf::Accessor* uvAccessor = nullptr;
	//			// glTF 通常使用 TEXCOORD_0 作為第一層 UV
	//			if (primitive.attributes.count("TEXCOORD_0")) {
	//				uvAccessor = &model.accessors.at(primitive.attributes.at("TEXCOORD_0"));
	//				const tinygltf::BufferView& uvBufferView = model.bufferViews[uvAccessor->bufferView];
	//				const tinygltf::Buffer& uvBuffer = model.buffers[uvBufferView.buffer];
	//				uvBufferStart = &uvBuffer.data[uvBufferView.byteOffset + uvAccessor->byteOffset];
	//				uvByteStride = uvAccessor->ByteStride(uvBufferView);
	//			}

	//			// --- 獲取頂點總數以進行邊界檢查 ---
	//			const size_t vertexCount = posAccessor.count;

	//			for (size_t j = 0; j < indexAccessor.count; j += 3) 
	//			{
	//				// 1. 在迴圈開始時進行初始化
	//				unsigned int i0 = 0, i1 = 0, i2 = 0;
	//				bool indices_valid = true; // 添加一個標誌來追蹤索引是否成功讀取

	//				// 2. 讀取索引，並在不支援的類型時設置標誌
	//				if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) 
	//				{
	//					const uint16_t* indices = reinterpret_cast<const uint16_t*>(&indexBufferData[j * sizeof(uint16_t)]);
	//					i0 = indices[0];
	//					i1 = indices[1];
	//					i2 = indices[2];
	//				}
	//				else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) 
	//				{
	//					const uint32_t* indices = reinterpret_cast<const uint32_t*>(&indexBufferData[j * sizeof(uint32_t)]);
	//					i0 = indices[0];
	//					i1 = indices[1];
	//					i2 = indices[2];
	//				}
	//				else 
	//				{
	//					// 如果索引類型不支援，設置標誌並準備跳過
	//					XENGINE_WARN("Unsupported index component type: {}", indexAccessor.componentType);
	//					indices_valid = false;
	//				}

	//				// 3. 檢查標誌，如果無效則跳過此三角形
	//				if (!indices_valid) 
	//				{
	//					continue;
	//				}

	//				// --- 新增：邊界檢查 (現在 i0, i1, i2 肯定是初始化的) ---
	//				if (i0 >= vertexCount || i1 >= vertexCount || i2 >= vertexCount)
	//				{
	//					XENGINE_WARN("Vertex index out of bounds! Index values: ({}, {}, {}), Vertex count: {}. Skipping triangle.", i0, i1, i2, vertexCount);
	//					continue; // 跳過這個無效的三角形
	//				}
	//				// --- 更安全地讀取頂點和法線 ---
	//				// 使用 byte-addressing，而不是假設它們是緊密排列的 float 陣列
	//				const float* v0_ptr = reinterpret_cast<const float*>(posBufferStart + i0 * posByteStride);
	//				const float* v1_ptr = reinterpret_cast<const float*>(posBufferStart + i1 * posByteStride);
	//				const float* v2_ptr = reinterpret_cast<const float*>(posBufferStart + i2 * posByteStride);

	//				glm::vec3 v0_local(v0_ptr[0], v0_ptr[1], v0_ptr[2]);
	//				glm::vec3 v1_local(v1_ptr[0], v1_ptr[1], v1_ptr[2]);
	//				glm::vec3 v2_local(v2_ptr[0], v2_ptr[1], v2_ptr[2]);

	//				// --- [新增] 讀取 UV 座標 ---
	//				glm::vec2 uv0(0.0f), uv1(0.0f), uv2(0.0f);
	//				if (uvBufferStart && uvAccessor) {
	//					// 注意邊界檢查
	//					if (i0 < uvAccessor->count) {
	//						const float* uv0_ptr = reinterpret_cast<const float*>(uvBufferStart + i0 * uvByteStride);
	//						uv0 = glm::vec2(uv0_ptr[0], uv0_ptr[1]);
	//					}
	//					if (i1 < uvAccessor->count) {
	//						const float* uv1_ptr = reinterpret_cast<const float*>(uvBufferStart + i1 * uvByteStride);
	//						uv1 = glm::vec2(uv1_ptr[0], uv1_ptr[1]);
	//					}
	//					if (i2 < uvAccessor->count) {
	//						const float* uv2_ptr = reinterpret_cast<const float*>(uvBufferStart + i2 * uvByteStride);
	//						uv2 = glm::vec2(uv2_ptr[0], uv2_ptr[1]);
	//					}
	//				}

	//				glm::vec3 n0_local(0.0f, 1.0f, 0.0f), n1_local(0.0f, 1.0f, 0.0f), n2_local(0.0f, 1.0f, 0.0f);
	//				if (normalBufferStart && normalAccessor && i0 < normalAccessor->count && i1 < normalAccessor->count && i2 < normalAccessor->count) {
	//					const float* n0_ptr = reinterpret_cast<const float*>(normalBufferStart + i0 * normalByteStride);
	//					const float* n1_ptr = reinterpret_cast<const float*>(normalBufferStart + i1 * normalByteStride);
	//					const float* n2_ptr = reinterpret_cast<const float*>(normalBufferStart + i2 * normalByteStride);
	//					n0_local = glm::vec3(n0_ptr[0], n0_ptr[1], n0_ptr[2]);
	//					n1_local = glm::vec3(n1_ptr[0], n1_ptr[1], n1_ptr[2]);
	//					n2_local = glm::vec3(n2_ptr[0], n2_ptr[1], n2_ptr[2]);
	//				}

	//				// --- 变换到世界空间 ---
	//				glm::vec3 v0_world = worldTransform * glm::vec4(v0_local, 1.0f);
	//				glm::vec3 v1_world = worldTransform * glm::vec4(v1_local, 1.0f);
	//				glm::vec3 v2_world = worldTransform * glm::vec4(v2_local, 1.0f);

	//				glm::vec3 n0_world = glm::normalize(normalTransform * n0_local);
	//				glm::vec3 n1_world = glm::normalize(normalTransform * n1_local);
	//				glm::vec3 n2_world = glm::normalize(normalTransform * n2_local);

	//				// --- 更新包围盒 ---
	//				m_boundsMin = glm::min(m_boundsMin, v0_world);
	//				m_boundsMin = glm::min(m_boundsMin, v1_world);
	//				m_boundsMin = glm::min(m_boundsMin, v2_world);
	//				m_boundsMax = glm::max(m_boundsMax, v0_world);
	//				m_boundsMax = glm::max(m_boundsMax, v1_world);
	//				m_boundsMax = glm::max(m_boundsMax, v2_world);

	//				// --- 存入 mMesh ---
	//				glm::vec4 v0_world_v4(v0_world, 1.0f);
	//				glm::vec4 v1_world_v4(v1_world, 1.0f);
	//				glm::vec4 v2_world_v4(v2_world, 1.0f);
	//				glm::ivec4 indice(-1, -1, -1, -1);
	//				// [重要修改] 去重現在必須檢查 位置 AND UV
	//				// 如果位置相同但 UV 不同，視為不同頂點 (例如貼圖接縫處)
	//				// 假設 mMesh 有一個 std::vector<glm::vec2> texCoords;
	//				for (int i = 0; i < mMesh.vertices.size(); i++) {
	//					// 這裡使用了 epsilon 比較 (glm::equal 可能需要定義 epsilon，或者直接比較)
	//					// 為了簡單起見，這裡假設 operator== 已正確重載或直接比較
	//					bool p0_match = (mMesh.vertices[i] == v0_world_v4) && (mMesh.texCoords[i] == uv0);
	//					bool p1_match = (mMesh.vertices[i] == v1_world_v4) && (mMesh.texCoords[i] == uv1);
	//					bool p2_match = (mMesh.vertices[i] == v2_world_v4) && (mMesh.texCoords[i] == uv2);

	//					if (p0_match) indice.x = i;
	//					if (p1_match) indice.y = i;
	//					if (p2_match) indice.z = i;

	//					if (indice.x != -1 && indice.y != -1 && indice.z != -1) {
	//						break;
	//					}
	//				}

	//				for (int i = 0; i < 3; i++)
	//				{
	//					if (indice[i] == -1) {
	//						indice[i] = static_cast<int>(mMesh.vertices.size());
	//						// 根據索引將對應的 位置 和 UV 存入
	//						if (i == 0) {
	//							mMesh.vertices.push_back(v0_world_v4);
	//							mMesh.texCoords.push_back(uv0); // [新增] 存入 UV
	//						}
	//						if (i == 1) {
	//							mMesh.vertices.push_back(v1_world_v4);
	//							mMesh.texCoords.push_back(uv1); // [新增] 存入 UV
	//						}
	//						if (i == 2) {
	//							mMesh.vertices.push_back(v2_world_v4);
	//							mMesh.texCoords.push_back(uv2); // [新增] 存入 UV
	//						}
	//					}
	//				}
	//				mMesh.indices.push_back(indice);
	//				glm::vec3 normal = glm::normalize(glm::cross(v1_world - v0_world, v2_world - v0_world));
	//				mMesh.normals.push_back(glm::vec4(normal, 0.0f));

	//				// 生成的Mesh記錄材質索引
	//				mMesh.materialIndices.push_back(materialIndex);
	//			}
	//		}
	//	}

	//	// 遞迴處理子節點
	//	for (size_t i = 0; i < node.children.size(); ++i) {
	//		extractNodeMesh(model, model.nodes[node.children[i]], worldTransform);
	//	}
