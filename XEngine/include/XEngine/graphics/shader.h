#pragma once
#include <string>
#include <fstream>
#include <sstream>
#include <optional>

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


		inline uint32_t getProgramId() const { return mProgramId; }

	private:
		int getUniformLoctaion(const std::string& name);
		void readFile(const char* shaderPath);


	private:
		uint32_t mProgramId;
		uint32_t mTexture;
		std::unordered_map<std::string, int> mUniformLocations;

		std::string shaderCode;
	};

}