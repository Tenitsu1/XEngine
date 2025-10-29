#version 460 core
out vec4 outColor;
in vec3 vpos;
in vec2 uvs;

uniform sampler2D tex;

void main()
{
	vec2 adjustedUVs = vec2(1.0 - uvs.y, uvs.x);
    outColor = texture(tex, adjustedUVs);
}