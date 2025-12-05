#include "graphics/gltfLoader.h"

#define TINYGLTF_IMPLEMENTATION
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
		if (!node.matrix.empty()) {
			return glm::make_mat4(node.matrix.data());
		}

		// 如果沒有 matrix，則根據 TRS (Translate, Rotate, Scale) 合成
		glm::mat4 transform = glm::mat4(1.0f);

		// 1. Scale
		if (!node.scale.empty()) {
			transform = glm::scale(transform, glm::vec3(node.scale[0], node.scale[1], node.scale[2]));
		}

		// 2. Rotate
		if (!node.rotation.empty()) {
			glm::quat q = glm::make_quat(node.rotation.data());
			transform = glm::mat4_cast(q) * transform;
		}

		// 3. Translate
		if (!node.translation.empty()) {
			transform = glm::translate(glm::mat4(1.0f), glm::vec3(node.translation[0], node.translation[1], node.translation[2])) * transform;
		}

		return transform;
	}

	GLTFStaticMesh::GLTFStaticMesh(tinygltf::Model& model, const char* filename)
	{
		tinygltf::TinyGLTF loader;
		std::string error;
		std::string warning;

		bool result = loader.LoadASCIIFromFile(&model, &error, &warning, filename);

		if (!warning.empty())    XENGINE_WARN("Warning: {}", warning);
		if (!error.empty())      XENGINE_ERROR("Error : {}", error);
		if (!result)             XENGINE_ERROR("Failed to load glTF : ", filename);
		

		XENGINE_TRACE("Loaded gltf : {}", filename);

		loadTextures(model);
		//createTextureArray(model);
		extractMaterials(model);
		//vaoAndEbos = bindModel(model);
		setupRenderPrimitives(model, generateVBOs(model));

		extractMesh(model);
		buildPackedTriangles();
	}

	GLTFStaticMesh::~GLTFStaticMesh()
	{
		// delete Global VAO
		glDeleteVertexArrays(1, &vaoAndEbos.first);

		// delete VAOs
		for (auto const& [key, val] : vaoAndEbos.second)
		{
			glDeleteBuffers(1, &val);
		}
		vaoAndEbos.second.clear(); 

		// delete all Per-Primitive VAOs with mRenderCache
		for (auto& [meshIndex, primitives] : mRenderCache) {
			for (auto& prim : primitives) {
				glDeleteVertexArrays(1, &prim.vao);
			}
		}
		mRenderCache.clear();

		// delete textures
		if (!mTextures.empty()) {
			glDeleteTextures((GLsizei)mTextures.size(), mTextures.data());
			mTextures.clear();
		}
	}


	void GLTFStaticMesh::bindMesh(std::map<int, GLuint>& vbos,
		tinygltf::Model& model, tinygltf::Mesh& mesh) {

		// 1. 遍歷 bufferViews，產生 VBO 並上傳數據 (這部分保留)
		for (int i = 0; i < model.bufferViews.size(); ++i) {
			const tinygltf::BufferView& bufferView = model.bufferViews[i];
			if (bufferView.target == 0) {
				// 有些 bufferView 是存 image 的，target 為 0，跳過是正確的，
				// 但為了安全，建議只針對 ARRAY_BUFFER 和 ELEMENT_ARRAY_BUFFER 處理
				continue;
			}

			// 如果這個 bufferView 還沒生成過 VBO (避免重複生成)
			if (vbos.find(i) == vbos.end()) {
				const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
				GLuint vbo;
				glGenBuffers(1, &vbo);
				vbos[i] = vbo;

				glBindBuffer(bufferView.target, vbo);
				glBufferData(bufferView.target, bufferView.byteLength,
					&buffer.data.at(0) + bufferView.byteOffset, GL_STATIC_DRAW);

				// 解除綁定，保持狀態乾淨
				glBindBuffer(bufferView.target, 0);
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

	// 修改原本的 bindModel 為 generateVBOs
	std::map<int, GLuint> GLTFStaticMesh::generateVBOs(tinygltf::Model& model) {
		std::map<int, GLuint> vbos;
		for (int i = 0; i < model.bufferViews.size(); ++i) {
			const tinygltf::BufferView& bufferView = model.bufferViews[i];
			if (bufferView.target == 0) continue;

			const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
			GLuint vbo;
			glGenBuffers(1, &vbo);
			vbos[i] = vbo;

			glBindBuffer(bufferView.target, vbo);
			glBufferData(bufferView.target, bufferView.byteLength,
				&buffer.data.at(0) + bufferView.byteOffset, GL_STATIC_DRAW);
		}
		glBindBuffer(GL_ARRAY_BUFFER, 0); // Reset
		return vbos;
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
		if (model.textures.empty()) return;

		mTextures.resize(model.textures.size());
		glGenTextures((GLsizei)model.textures.size(), mTextures.data());

		for (size_t i = 0; i < model.textures.size(); i++) {
			const tinygltf::Texture& tex = model.textures[i];
			if (tex.source < 0) continue;

			glBindTexture(GL_TEXTURE_2D, mTextures[i]);

			const tinygltf::Image& image = model.images[tex.source];

			GLenum format = GL_RGBA;
			if (image.component == 1) format = GL_RED;
			else if (image.component == 2) format = GL_RG;
			else if (image.component == 3) format = GL_RGB;

			GLenum internalFormat = (format == GL_RGBA) ? GL_SRGB_ALPHA : GL_SRGB;
			GLenum type = (image.bits == 16) ? GL_UNSIGNED_SHORT : GL_UNSIGNED_BYTE;

			glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, image.width, image.height, 0, format, type, &image.image.at(0));
			glGenerateMipmap(GL_TEXTURE_2D);

			// Sampler settings
			GLint minFilter = GL_LINEAR_MIPMAP_LINEAR;
			GLint magFilter = GL_LINEAR;
			GLint wrapS = GL_REPEAT;
			GLint wrapT = GL_REPEAT;

			if (tex.sampler >= 0) {
				const tinygltf::Sampler& sampler = model.samplers[tex.sampler];
				if (sampler.minFilter != -1) minFilter = sampler.minFilter;
				if (sampler.magFilter != -1) magFilter = sampler.magFilter;
				wrapS = sampler.wrapS;
				wrapT = sampler.wrapT;
			}

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapS);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapT);
		}
		glBindTexture(GL_TEXTURE_2D, 0);
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

	// Helper function for recursive traversal
	void printNodeMaterialInfo(const tinygltf::Model& model, const tinygltf::Node& node, int depth) {
		// --- Print Node Info ---
		std::string indent(depth * 2, ' '); // Indentation for hierarchy
		// 使用 C++20 的 std::format 或者 fmtlib，XENGINE_TRACE 內部已經封裝好了
		XENGINE_TRACE("{}[Node {}] '{}'", indent, (&node - &model.nodes[0]), (node.name.empty() ? "unnamed" : node.name));

		// --- Check if Node has a Mesh ---
		if (node.mesh >= 0 && node.mesh < model.meshes.size()) {
			const tinygltf::Mesh& mesh = model.meshes[node.mesh];
			XENGINE_TRACE("{}  -> Mesh {}: '{}'", indent, node.mesh, (mesh.name.empty() ? "unnamed" : mesh.name));

			// --- Iterate through Mesh Primitives ---
			for (size_t i = 0; i < mesh.primitives.size(); ++i) {
				const tinygltf::Primitive& primitive = mesh.primitives[i];
				XENGINE_TRACE("{}    - Primitive {}:", indent, i);

				// --- Get Material Info ---
				if (primitive.material >= 0 && primitive.material < model.materials.size()) {
					const tinygltf::Material& material = model.materials[primitive.material];
					XENGINE_TRACE("{}      - Material {}: '{}'", indent, primitive.material, (material.name.empty() ? "unnamed" : material.name));

					// --- Get Texture Info ---
					int textureIndex = material.pbrMetallicRoughness.baseColorTexture.index;
					if (textureIndex >= 0 && textureIndex < model.textures.size()) {
						const tinygltf::Texture& texture = model.textures[textureIndex];
						XENGINE_TRACE("{}        - BaseColorTexture {}", indent, textureIndex);

						// --- Get Image Info ---
						if (texture.source >= 0 && texture.source < model.images.size()) {
							const tinygltf::Image& image = model.images[texture.source];
							XENGINE_TRACE("{}          -> Image {}: '{}'", indent, texture.source, (image.uri.empty() ? "embedded" : image.uri));
						}
						else {
							XENGINE_WARN("{}          -> No Image source found for Texture {}.", indent, textureIndex);
						}
					}
					else {
						XENGINE_TRACE("{}        - No BaseColorTexture assigned.", indent);
					}

				}
				else {
					XENGINE_WARN("{}      - No Material assigned.", indent);
				}
			}
		}

		// --- Recurse into Children ---
		for (size_t i = 0; i < node.children.size(); ++i) {
			int childNodeIndex = node.children[i];
			if (childNodeIndex >= 0 && childNodeIndex < model.nodes.size()) {
				printNodeMaterialInfo(model, model.nodes[childNodeIndex], depth + 1);
			}
		}
	}


	// The public member function implementation, now using XENGINE_TRACE
	void XEngine::graphics::GLTFStaticMesh::printMaterialTextureMapping(const tinygltf::Model& model) {
		XENGINE_TRACE(""); // Print an empty line for spacing
		XENGINE_INFO("========================================");
		XENGINE_INFO("  glTF Material-Texture Mapping Report");
		XENGINE_INFO("========================================");

		if (model.scenes.empty()) {
			XENGINE_WARN("No scenes found in the model.");
			return;
		}

		// Start traversal from the root nodes of the default scene
		const tinygltf::Scene& scene = model.scenes[model.defaultScene > -1 ? model.defaultScene : 0];
		XENGINE_INFO("Processing Scene {}: '{}'\n", (model.defaultScene > -1 ? model.defaultScene : 0), scene.name);

		for (size_t i = 0; i < scene.nodes.size(); ++i) {
			int rootNodeIndex = scene.nodes[i];
			if (rootNodeIndex >= 0 && rootNodeIndex < model.nodes.size()) {
				printNodeMaterialInfo(model, model.nodes[rootNodeIndex], 0);
			}
		}

		XENGINE_INFO("========================================");
		XENGINE_INFO("          End of Report");
		XENGINE_INFO("========================================");
		XENGINE_TRACE(""); // Print an empty line for spacing
	}


	void GLTFStaticMesh::extractMesh(tinygltf::Model& model)
	{
		// 清空所有 Mesh 數據
		mMesh.vertices.clear();
		mMesh.faceNormals.clear();
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
				if (primitive.indices < 0 || primitive.attributes.find("POSITION") == primitive.attributes.end())
				{
					continue; // 跳過沒有索引或沒有頂點位置的 primitive
				}


				// --- 獲取數據訪問器 (Accessor) ---
				const tinygltf::Accessor& indexAccessor = model.accessors[primitive.indices];
				const tinygltf::Accessor& posAccessor = model.accessors.at(primitive.attributes.at("POSITION"));
				const tinygltf::Accessor* normalAccessor = nullptr;
				const tinygltf::Accessor* uvAccessor = nullptr;

				if (primitive.attributes.count("NORMAL"))
				{
					normalAccessor = &model.accessors.at(primitive.attributes.at("NORMAL"));
				}

				if (primitive.attributes.count("TEXCOORD_0"))
				{
					uvAccessor = &model.accessors.at(primitive.attributes.at("TEXCOORD_0"));
				}
				else
				{
					XENGINE_WARN("Mesh '{}', Primitive {} has no texture coordinates (TEXCOORD_0).", mesh.name, p);
				}

				// Position Buffer Views
				const tinygltf::BufferView& posBufferView = model.bufferViews[posAccessor.bufferView];
				const tinygltf::Buffer& posBuffer = model.buffers[posBufferView.buffer];
				const uint8_t* posBufferStart = &posBuffer.data[posBufferView.byteOffset + posAccessor.byteOffset];
				size_t posByteStride = posAccessor.ByteStride(posBufferView);

				// Normal Buffer Views
				const uint8_t* normalBufferStart = nullptr;
				size_t normalByteStride = 0;
				if (normalAccessor) {
					const tinygltf::BufferView& normalBufferView = model.bufferViews[normalAccessor->bufferView];
					normalBufferStart = &model.buffers[normalBufferView.buffer].data[normalBufferView.byteOffset + normalAccessor->byteOffset];
					normalByteStride = normalAccessor->ByteStride(normalBufferView);
				}

				// UV Buffer Views
				const uint8_t* uvBufferStart = nullptr;
				size_t uvByteStride = 0;
				if (uvAccessor) {
					const tinygltf::BufferView& uvBufferView = model.bufferViews[uvAccessor->bufferView];
					uvBufferStart = &model.buffers[uvBufferView.buffer].data[uvBufferView.byteOffset + uvAccessor->byteOffset];
					uvByteStride = uvAccessor->ByteStride(uvBufferView);
				}


				// Vertices in primitive, and push back mMesh
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


					if (normalAccessor && normalBufferStart) {
						const float* n_ptr = reinterpret_cast<const float*>(normalBufferStart + v_idx * normalByteStride);
						glm::vec3 n_local(n_ptr[0], n_ptr[1], n_ptr[2]);
						glm::vec3 n_world = glm::normalize(normalTransform * n_local);
						mMesh.normals.push_back(glm::vec4(n_world, 0.0f));
					}
					else {
						// 如果模型沒有提供法線，填充一個預設值
						mMesh.normals.push_back(glm::vec4(0.0f, 1.0f, 0.0f, 0.0f));
					}

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

				// Indices
				const tinygltf::BufferView& indexBufferView = model.bufferViews[indexAccessor.bufferView];
				const uint8_t* indexBufferData = &model.buffers[indexBufferView.buffer].data[indexBufferView.byteOffset + indexAccessor.byteOffset];
				int materialIndex = primitive.material;
				bool doubleSided = (materialIndex >= 0) ? model.materials[materialIndex].doubleSided : false;


				// 遍歷這個 primitive 的所有索引
				for (size_t j = 0; j < indexAccessor.count; j += 3) {
					unsigned int i0, i1, i2;

					// 使用 switch 簡化索引讀取
					switch (indexAccessor.componentType) {
					case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
						const uint16_t* indices = reinterpret_cast<const uint16_t*>(&indexBufferData[j * sizeof(uint16_t)]);
						i0 = indices[0]; i1 = indices[1]; i2 = indices[2];
						break;
					}
					case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: {
						const uint32_t* indices = reinterpret_cast<const uint32_t*>(&indexBufferData[j * sizeof(uint32_t)]);
						i0 = indices[0]; i1 = indices[1]; i2 = indices[2];
						break;
					}
					default:
						XENGINE_WARN("Unsupported index component type: {}", indexAccessor.componentType);
						continue; // 跳過此三角形
					}

					// 將局部索引加上基底偏移量，得到全局索引
					glm::ivec4 global_indices(
						i0 + vertex_offset,
						i1 + vertex_offset,
						i2 + vertex_offset,
						(int)doubleSided
					);
					mMesh.indices.push_back(global_indices);

					// 為這個新生成的三角形記錄材質索引
					mMesh.materialIndices.push_back(materialIndex);

					// 計算面法線並追加
					// 注意：法線現在是按面計算的，所以 normals 數組長度會和 indices/materialIndices 一樣
					glm::vec3 v0 = mMesh.vertices[global_indices.x];
					glm::vec3 v1 = mMesh.vertices[global_indices.y];
					glm::vec3 v2 = mMesh.vertices[global_indices.z];
					glm::vec3 normal = glm::normalize(glm::cross(v1 - v0, v2 - v0));
					mMesh.faceNormals.push_back(glm::vec4(normal, 0.0f));
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
	void GLTFStaticMesh::extractMaterials(tinygltf::Model& model)
	{
		Material mat;
		size_t matCount = model.materials.size();
		mMaterials.clear();
		mMaterials.reserve(matCount);

		for (size_t i = 0; i < matCount; ++i)
		{
			const tinygltf::Material& gltfMat = model.materials[i];

			// 1. Base Color
			if (gltfMat.pbrMetallicRoughness.baseColorFactor.size() == 4) {
				mat.baseColorFactor = glm::make_vec4(gltfMat.pbrMetallicRoughness.baseColorFactor.data());
			}
			mat.baseColorTexture = gltfMat.pbrMetallicRoughness.baseColorTexture.index;

			// 2. Metallic & Roughness
			mat.metallicFactor = (float)gltfMat.pbrMetallicRoughness.metallicFactor;
			mat.roughnessFactor = (float)gltfMat.pbrMetallicRoughness.roughnessFactor;
			mat.metallicRoughnessTexture = gltfMat.pbrMetallicRoughness.metallicRoughnessTexture.index;

			// 3. Normal
			mat.normalTexture = gltfMat.normalTexture.index;

			// 4. Emissive (自發光)
			auto emissiveStrength = gltfMat.extensions.find("KHR_materials_emissive_strength");
			if (gltfMat.emissiveFactor.size() == 3) {
				if (emissiveStrength != gltfMat.extensions.end() && emissiveStrength->second.IsObject()) {
					const auto& val = emissiveStrength->second;
					if (val.Has("emissiveStrength")) {
						/*mat.emissionFactor = glm::vec4(glm::make_vec3(gltfMat.emissiveFactor.data()), (float)val.Get("emissiveStrength").GetNumberAsDouble());*/
						mat.emissionFactor = glm::vec4(glm::make_vec3(gltfMat.emissiveFactor.data()), 10);
					}
				}				
			}

			// 5. Extensions (Transmission & IOR)
			auto transmissionIt = gltfMat.extensions.find("KHR_materials_transmission");
			if (transmissionIt != gltfMat.extensions.end() && transmissionIt->second.IsObject()) {
				const auto& val = transmissionIt->second;
				if (val.Has("transmissionFactor")) {
					mat.transmissionFactor = (float)val.Get("transmissionFactor").GetNumberAsDouble();
				}
			}

			auto iorIt = gltfMat.extensions.find("KHR_materials_ior");
			if (iorIt != gltfMat.extensions.end() && iorIt->second.IsObject()) {
				const auto& val = iorIt->second;
				if (val.Has("ior")) {
					mat.ior = (float)val.Get("ior").GetNumberAsDouble();
				}
			}

			mMaterials.push_back(mat);
		}

		XENGINE_TRACE("Extracted {} materials (AoS format)", matCount);
	}

	void GLTFStaticMesh::drawWithShader(std::shared_ptr<XEngine::Shader> shader, tinygltf::Model& model) {
		if (model.scenes.empty()) return;

		const tinygltf::Scene& scene = model.scenes[model.defaultScene > -1 ? model.defaultScene : 0];

		// 遞迴繪製場景樹
		for (size_t i = 0; i < scene.nodes.size(); ++i) {
			drawNodeRecursive(shader, model, scene.nodes[i], glm::mat4(1.0f));
		}

	}

	// =========================================================================
	// 遞迴邏輯
	// =========================================================================
	void GLTFStaticMesh::drawNodeRecursive(std::shared_ptr<XEngine::Shader> shader, tinygltf::Model& model, int nodeIdx, const glm::mat4& parentTransform) {
		const tinygltf::Node& node = model.nodes[nodeIdx];

		glm::mat4 localTransform = GetLocalMatrix(node);
		glm::mat4 globalTransform = parentTransform * localTransform;

		if (node.mesh >= 0) {
			shader->setUniformMat4("model", globalTransform);
			drawMeshWithMaterial(shader, model, model.meshes[node.mesh]);
		}

		for (int childIdx : node.children) {
			drawNodeRecursive(shader, model, childIdx, globalTransform);
		}
	}

	// =========================================================================
	// 材質綁定與繪製
	// =========================================================================
	void GLTFStaticMesh::drawMeshWithMaterial(std::shared_ptr<XEngine::Shader> shader, tinygltf::Model& model, tinygltf::Mesh& mesh) {

		// 我們需要知道這是第幾個 Mesh 才能去查 mRenderCache
		// 由於 tinygltf::Mesh 結構本身不帶 ID，這裡需要從 model.meshes 指針推算 ID
		// 或者修改 drawMeshWithMaterial 讓它傳入 meshIndex
		// 這裡使用指針運算來獲取 index (假設 mesh 是來自 model.meshes 的引用)
		int meshIndex = (int)(&mesh - &model.meshes[0]);

		if (mRenderCache.find(meshIndex) == mRenderCache.end()) return;

		const auto& primitives = mRenderCache[meshIndex];

		// --- Material Setup ---
		glm::vec4 baseColor(1.0f);
		glm::vec4 emission(0.0f);
		float metallic = 1.0f;
		float roughness = 1.0f;
		float ior = 1.5f;
		float transmission = 0.0f;

		bool useBaseColorMap = false;
		bool useMetallicRoughnessMap = false;
		bool useNormalMap = false;
		bool useEmissiveMap = false;

		for (const auto& renderPrim : primitives) {
			if (renderPrim.materialIndex >= 0) {
				const tinygltf::Material& mat = model.materials[renderPrim.materialIndex];

				// Base Color
				if (mat.pbrMetallicRoughness.baseColorFactor.size() == 4) {
					baseColor = glm::make_vec4(mat.pbrMetallicRoughness.baseColorFactor.data());
				}
				if (mat.pbrMetallicRoughness.baseColorTexture.index >= 0) {
					glActiveTexture(GL_TEXTURE0);
					glBindTexture(GL_TEXTURE_2D, mTextures[mat.pbrMetallicRoughness.baseColorTexture.index]);
					useBaseColorMap = true;
				}

				// Metallic & Roughness
				metallic = (float)mat.pbrMetallicRoughness.metallicFactor;
				roughness = (float)mat.pbrMetallicRoughness.roughnessFactor;
				if (mat.pbrMetallicRoughness.metallicRoughnessTexture.index >= 0) {
					glActiveTexture(GL_TEXTURE1);
					glBindTexture(GL_TEXTURE_2D, mTextures[mat.pbrMetallicRoughness.metallicRoughnessTexture.index]);
					useMetallicRoughnessMap = true;
				}

				// Normal
				if (mat.normalTexture.index >= 0) {
					glActiveTexture(GL_TEXTURE2);
					glBindTexture(GL_TEXTURE_2D, mTextures[mat.normalTexture.index]);
					useNormalMap = true;
				}

				// Emissive
				if (mat.emissiveFactor.size() == 3) {
					emission = glm::vec4(glm::make_vec3(mat.emissiveFactor.data()), 1.0f);
				}
				if (mat.extensions.count("KHR_materials_emissive_strength")) {
					const auto& ext = mat.extensions.at("KHR_materials_emissive_strength");
					if (ext.Has("emissiveStrength")) {
						float strength = (float)ext.Get("emissiveStrength").GetNumberAsDouble();
						emission *= strength; 
					}
				}
				if (mat.emissiveTexture.index >= 0) {
					glActiveTexture(GL_TEXTURE3);
					glBindTexture(GL_TEXTURE_2D, mTextures[mat.emissiveTexture.index]);
					useEmissiveMap = true;
				}

				// --- Draw Call ---
				glBindVertexArray(renderPrim.vao);

				glDrawElements(renderPrim.mode,
					renderPrim.count,
					renderPrim.type,
					BUFFER_OFFSET(renderPrim.byteOffset));
			}
		}

		// Upload Uniforms
		shader->setUniformFloat4("material.baseColorFactor", baseColor);
		shader->setUniformFloat4("material.emissionFactor", emission);
		shader->setUniformFloat1("material.metallicFactor", metallic);
		shader->setUniformFloat1("material.roughnessFactor", roughness);
		
		shader->setUniformBool("material.useBaseColorMap", useBaseColorMap);
		shader->setUniformBool("material.useMetallicRoughnessMap", useMetallicRoughnessMap);
		shader->setUniformBool("material.useNormalMap", useNormalMap);
		shader->setUniformBool("material.useEmissiveMap", useEmissiveMap);

		glBindVertexArray(0);
	}

	// 這是優化版本的 processMesh，請放在初始化階段呼叫
	void GLTFStaticMesh::setupRenderPrimitives(tinygltf::Model& model, std::map<int, GLuint>& bufferViewVBOs) {

		for (int i = 0; i < model.meshes.size(); ++i) {
			const auto& mesh = model.meshes[i];
			std::vector<RenderPrimitive> primitives;

			for (const auto& primitive : mesh.primitives) {
				RenderPrimitive renderPrim;
				renderPrim.materialIndex = primitive.material;
				renderPrim.mode = primitive.mode;

				// 1. 生成並綁定 VAO (開始錄製狀態)
				glGenVertexArrays(1, &renderPrim.vao);
				glBindVertexArray(renderPrim.vao);

				// 2. 設定 VBO 屬性 (這些設定會被記錄在 VAO 中)
				for (const auto& attrib : primitive.attributes) {
					const std::string& name = attrib.first;
					int accessorIdx = attrib.second;
					const auto& accessor = model.accessors[accessorIdx];
					const auto& bufferView = model.bufferViews[accessor.bufferView];

					// 綁定 VBO
					glBindBuffer(GL_ARRAY_BUFFER, bufferViewVBOs[accessor.bufferView]);

					// 設定 Location (根據你的 Shader Layout)
					int location = -1;
					if (name == "POSITION") location = 0;
					else if (name == "NORMAL") location = 1;
					else if (name == "TEXCOORD_0") location = 2;
					// else if (name == "TANGENT") location = 3; 

					if (location != -1) {
						glEnableVertexAttribArray(location);
						glVertexAttribPointer(location,
							(accessor.type == TINYGLTF_TYPE_SCALAR) ? 1 : accessor.type,
							accessor.componentType,
							accessor.normalized ? GL_TRUE : GL_FALSE,
							accessor.ByteStride(bufferView),
							BUFFER_OFFSET(accessor.byteOffset));
					}
				}

				// 3. 綁定 EBO (EBO 的綁定狀態也會被記錄在 VAO 中！)
				if (primitive.indices >= 0) {
					const auto& indexAccessor = model.accessors[primitive.indices];
					//const auto& indexBufferView = model.bufferViews[indexAccessor.bufferView];

					glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufferViewVBOs[indexAccessor.bufferView]);

					renderPrim.count = (GLsizei)indexAccessor.count;
					renderPrim.type = indexAccessor.componentType;
					renderPrim.byteOffset = (uint32_t)indexAccessor.byteOffset;
				}

				// 4. 結束錄製
				glBindVertexArray(0);

				// 重要：解綁 VBO/EBO (VAO 已經記住了，這裡解綁是為了不影響外部狀態)
				glBindBuffer(GL_ARRAY_BUFFER, 0);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

				primitives.push_back(renderPrim);
			}

			mRenderCache[i] = primitives;
		}
	}

	void GLTFStaticMesh::buildPackedTriangles() {
		XENGINE_TRACE("Building Packed Triangles...");

		size_t triCount = mMesh.indices.size();
		mPackedTriangles.clear();
		mPackedTriangles.reserve(triCount);

		for (const auto& idx : mMesh.indices) {
			glm::vec3 p0 = glm::vec3(mMesh.vertices[idx.x]);
			glm::vec3 p1 = glm::vec3(mMesh.vertices[idx.y]);
			glm::vec3 p2 = glm::vec3(mMesh.vertices[idx.z]);

			PackedTriangle tri;
			tri.v0 = glm::vec4(p0, (float)idx.w); // w stores doubleSided
			tri.e1 = glm::vec4(p1 - p0, 0.0f);
			tri.e2 = glm::vec4(p2 - p0, 0.0f);
			mPackedTriangles.push_back(tri);
		}

		XENGINE_TRACE("Packed {} triangles.", mPackedTriangles.size());
	}

	void GLTFStaticMesh::createTextureArray(const tinygltf::Model& model) {
		if (model.textures.empty()) return;

		int maxWidth = 0;
		int maxHeight = 0;

		for (const auto& tex : model.textures) {
			if (tex.source < 0 || tex.source >= model.images.size()) continue;
			const auto& img = model.images[tex.source];
			maxWidth = std::max(maxWidth, img.width);
			maxHeight = std::max(maxHeight, img.height);
		}

		if (maxWidth == 0) { maxWidth = 1024; maxHeight = 1024; }

		int layerCount = (int)model.textures.size();

		glGenTextures(1, &mTextureArrayID);
		glBindTexture(GL_TEXTURE_2D_ARRAY, mTextureArrayID);
		glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, maxWidth, maxHeight, layerCount, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

		GLuint readFBO, drawFBO;
		glGenFramebuffers(1, &readFBO);
		glGenFramebuffers(1, &drawFBO);

		for (int i = 0; i < layerCount; ++i) {
			int sourceImgIdx = model.textures[i].source;
			if (sourceImgIdx < 0 || sourceImgIdx >= model.images.size()) continue;

			const auto& img = model.images[sourceImgIdx];

			GLuint tempTex;
			glGenTextures(1, &tempTex);
			glBindTexture(GL_TEXTURE_2D, tempTex);

			GLenum format = GL_RGBA;
			if (img.component == 1) format = GL_RED;
			else if (img.component == 3) format = GL_RGB;
			else if (img.component == 4) format = GL_RGBA;

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			glTexImage2D(GL_TEXTURE_2D, 0, format, img.width, img.height, 0, format, GL_UNSIGNED_BYTE, img.image.data());
			glBindTexture(GL_TEXTURE_2D, 0);

	
			glBindFramebuffer(GL_READ_FRAMEBUFFER, readFBO);
			glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tempTex, 0);

			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, drawFBO);
			glFramebufferTextureLayer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, mTextureArrayID, 0, i);

			glBlitFramebuffer(0, 0, img.width, img.height,   
				0, 0, maxWidth, maxHeight,
				GL_COLOR_BUFFER_BIT, GL_LINEAR);

			glDeleteTextures(1, &tempTex);
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glDeleteFramebuffers(1, &readFBO);
		glDeleteFramebuffers(1, &drawFBO);

		glBindTexture(GL_TEXTURE_2D_ARRAY, mTextureArrayID);
		glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
		glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

		//return mTextureArrayID;
	}
}
