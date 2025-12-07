#pragma once
#include <external/glm/glm.hpp>
#include <external/glm/gtc/matrix_transform.hpp>
#include <vector>

static const int InvalidID = -1;


struct Mesh
{
	std::vector<glm::vec4>  vertices;
	std::vector<glm::vec4>  faceNormals;
	std::vector<glm::vec4>  normals;
	std::vector<glm::ivec4> indices;
	std::vector<glm::vec2>  texCoords;
	std::vector<int>        materialIndices;
};

struct Material
{
	//  Material的參數定義參考於gltf官網以及blender內建參數

	//  --- 參考影片 ---
	//	Thanks for ZACK 3D
	//  (15分鐘學會PBR材質－觀念篇) https://www.youtube.com/watch?v=7viU875f2rw
	//  ---------------

	//  顏色一般來說有多種變體名稱，分別為
	//  1. Color
	//  2. BaseColor
	//  3. Diffuse
	//  4. Albedo(較為少見，通常會獨立出來作為反射紋理或者是由其他參數決定)
	glm::vec4 baseColorFactor;      // 基礎顏色因子　　　　Offset 0

	//  emission可能作為PBR底下的參數
	//  也有可能作為KHR_materials_emissive_strength的獨立參數
	//  其中RGB為發光顏色，A設定為發光強度
	glm::vec4 emissionFactor;       // 發光顏色或強度　　　Offset 16 

	float metallicFactor;           // 金屬度因子　　　　　Offset 32

	//  roughness(粗糙度)為常見的表達形式
	//  另一種變體為Glossiness(光澤度)，則為粗糙度的反向
	float roughnessFactor;          // 粗糙度因子　　　　　Offset 36
	float transmissionFactor;       // 透射率　　　　　　　Offset 40
	float ior;                      // 折射率　　　　　　　Offset 44

	int baseColorTexture;           // 基礎顏色紋理　　　　Offset 48
	int metallicRoughnessTexture;   // 金屬度與粗糙度紋理　Offset 52
	int normalTexture;              // 法向量紋理　　　　　Offset 56
	int emissiveTexture;            // 自發光紋理　　　　　Offset 60 (0: PBR, 1: Light)



	// initialize
	Material() {
		baseColorFactor = glm::vec4(1.0f);
		emissionFactor = glm::vec4(0.0f);
		metallicFactor = 1.0f;
		roughnessFactor = 1.0f;
		transmissionFactor = 0.0f;
		ior = 1.5f;
		baseColorTexture = -1;
		metallicRoughnessTexture = -1;
		normalTexture = -1;
		emissiveTexture = -1;
	}

};

struct RenderPrimitive {
	//uint32_t vao;           // 每個 Primitive 都有自己的 VAO
	//uint32_t count;         // 索引數量
	//uint32_t type;          // 索引類型 (unsigned short/int)
	//size_t byteOffset;    // EBO 偏移
	//int materialIndex;    // 材質索引
	uint32_t vao;           // 已經設定好屬性的 VAO
	uint32_t materialIndex; // 材質索引

	// 繪製參數 (直接存下來，繪製時不用再查 accessor)
	uint32_t mode;          // GL_TRIANGLES
	uint32_t  count;        // 索引數量
	uint32_t type;          // GL_UNSIGNED_SHORT / INT
	uint32_t byteOffset;    // EBO 偏移量
};

struct PackedTriangle {
	glm::vec4 v0;   // w = materialIndex (cast to float)
	glm::vec4 e1;   // w = padding
	glm::vec4 e2;   // w = padding
};
