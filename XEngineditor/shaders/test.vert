#version 460 core

layout (location = 0) in vec3 vertexPosition;
layout (location = 1) in vec2 vertexTexCoords;
// layout (location = 2) in vec3 vertexNormal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 vpos;
out vec2 uvs;

void main()
{
	uvs = vertexTexCoords;
	gl_Position = vec4(vertexPosition, 1.0);
}