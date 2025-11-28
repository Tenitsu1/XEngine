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
	glm::vec4 baseColorFactor;      // 基礎顏色因子　　　　Offset 0
	glm::vec4 emissionFactor;       // 發光顏色或強度　　　Offset 16 

	float metallicFactor;           // 金屬度因子　　　　　Offset 32
	float roughnessFactor;          // 粗糙度因子　　　　　Offset 36
	float transmissionFactor;       // 透射率　　　　　　　Offset 40
	float ior;                      // 折射率　　　　　　　Offset 44

	int baseColorTexture;           // 基礎顏色紋理　　　　Offset 48
	int metallicRoughnessTexture;   // 金屬度與粗糙度紋理　Offset 52
	int normalTexture;              // 法向量貼圖　　　　　Offset 56
	int type;                       // 是否為光源　　　　　Offset 60 (0: PBR, 1: Light)



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
		type = 0;
	}

};
