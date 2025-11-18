#pragma once
#include <string>
#include <fstream>
#include <sstream>
#include <optional>

#include "external/glm/glm.hpp"

#include <unordered_map>

namespace XEngine
{

	class Shader
	{
	public:
		Shader(const char* vertexPath, const char* fragmentPath);
		~Shader();

		void bind(const float* vertexArray, uint32_t vertexCount, uint32_t dimensions);
		void unbind();

		void draw(int width, int height);

		void setVAO();
		void setVBO();
		void setEBO(const void* data, size_t size);
		void setFBO();

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

		void bindTexture(uint32_t texture, uint32_t textureUnit);
		void createTexture(int width, int height);

		inline uint32_t getProgramId() const { return mProgramId; }
		inline uint32_t getTexture() const { return mTexture; }

	private:
		int getUniformLoctaion(const std::string& name);
		void readFile(const char* shaderPath);


	private:
		uint32_t mProgramId;
		uint32_t mTexture;
		uint32_t VAO, VBO, EBO, FBO;
		std::unordered_map<std::string, int> mUniformLocations;

		std::string shaderCode;
	};

}