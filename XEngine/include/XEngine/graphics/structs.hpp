#pragma once
#include <external/glm/glm.hpp>
#include <external/glm/gtc/matrix_transform.hpp>
#include <vector>

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
	glm::vec3 faceNormal; //offset 96  // alignment 16 // size 12 // total 108 bytes
	float padding7;     //offset 108 // alignment 4  // size 4  // total 112 bytes
};

struct Mesh
{
	std::vector<glm::vec4>  vertices;
	std::vector<glm::vec4>  normals;
	std::vector<glm::ivec4> indices;
	std::vector<glm::vec2>  texCoords;
};

struct Camera
{
	glm::vec3 position;
	glm::vec3 lookat;
	glm::vec3 up;

	glm::vec3 direction = glm::normalize(position - lookat);
	glm::vec3 right = glm::normalize(glm::cross(up, direction));
	glm::vec3 cameraUp = glm::cross(direction, right);

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