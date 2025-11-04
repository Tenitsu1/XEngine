#pragma once
#include<string>
#include<fstream>
#include<sstream>

#include "external/glm/glm.hpp"

#include <unordered_map>

namespace XEngine::graphics
{
	class Shader
	{
	public:
		Shader(const char* vertexPath, const char* fragmentPath);
		~Shader();

		void bind();
		void unbind();

		void setUniformInt(const std::string& name, int val);
		void setUniformFloat1(const std::string& name, float val1);
		void setUniformFloat2(const std::string& name, float val1, float val2);
		void setUniformFloat3(const std::string& name, float val1, float val2, float val3);
		void setUniformFloat4(const std::string& name, float val1, float val2, float val3, float val4);

		void setUniformFloat2(const std::string& name, const glm::vec2& val);
		void setUniformFloat3(const std::string& name, const glm::vec3& val);
		void setUniformFloat4(const std::string& name, const glm::vec4& val);

		void setUniformMat3(const std::string& name, const glm::mat3& mat);
		void setUniformMat4(const std::string& name, const glm::mat4& mat);

	private:
		int getUniformLoctaion(const std::string& name);
		void readFile(const char* shaderPath);


	private:
		uint32_t mProgramId;
		uint32_t mTexture;
		std::unordered_map<std::string, int> mUniformLocations;
		
		std::string shaderCode;
	};

	class ComputeShader
	{
	public:
		ComputeShader(const char* computePath, int width, int height);
		~ComputeShader();

		void createSSBO(uint32_t& ssbo, uint32_t size, const void* data, uint32_t binding);
		void DispatchCompute();

		void bind();
		void unbind();

	private:
		void readFile(const char* shaderPath);
		std::string shaderCode;
		uint32_t mTexture;
		uint32_t mProgramId;
		uint32_t mWidth;
		uint32_t mHeight;
	};

}