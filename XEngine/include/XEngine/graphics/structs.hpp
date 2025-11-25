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

struct Meterial
{
	std::vector<glm::vec4>  baseColorFactor;             // 基礎顏色因子
	std::vector<int>        baseColorTexture;            // 基礎顏色紋理
	std::vector<float>      metallicFactor;              // 金屬度因子
	std::vector<float>      roughnessFactor;             // 粗糙度因子
	std::vector<int>        metallicRoughnessTexture;    // 金屬度-粗糙度紋理
	std::vector<int>        normalTexture;               // 法向量貼圖
	std::vector<float>      transmissionFactor;          // 透射率
	std::vector<int>        ior;                         // 折射率 
};


struct Camera
{
	glm::vec3 position;
	glm::vec3 lookat;
	glm::vec3 up;

	glm::vec3 direction;
	glm::vec3 right;
	glm::vec3 cameraUp;

	glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f),
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f));

	float fov;
	float yaw   = 0.0f;
	float pitch = 0.0f;
	// camera options
	float MovementSpeed;
	float MouseSensitivity;
	float Zoom;

};