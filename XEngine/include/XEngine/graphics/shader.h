#pragma once
#include<string>
#include<fstream>
#include<sstream>

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

		void readFile(const char* vertexPath, const char* fragmentPath);

		void setUniformInt(const std::string& name, int val);
		void setUniformFloat1(const std::string& name, float val1);
		void setUniformFloat2(const std::string& name, float val1, float val2);
		void setUniformFloat3(const std::string& name, float val1, float val2, float val3);
		void setUniformFloat4(const std::string& name, float val1, float val2, float val3, float val4);


	private:
		int getUniformLoctaion(const std::string& name);


	private:
		uint32_t mProgramId;
		std::unordered_map<std::string, int> mUniformLocations;
		std::string vertexCode;
		std::string fragmentCode;
	};

}