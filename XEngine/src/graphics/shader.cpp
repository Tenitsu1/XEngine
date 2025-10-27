#include "graphics/shader.h"
#include "log.h"
#include "graphics/helper.h"

#include "glad/glad.h"


namespace XEngine::graphics
{
	// Read file
	void Shader::readFile(const char* shaderPath)
	{
		std::ifstream shaderFile;
		shaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		try
		{
			shaderFile.open(shaderPath);
			if (!shaderFile.is_open()) {
				throw std::runtime_error("Could not open shader file.");
			}

			std::stringstream shaderStream;
			shaderStream << shaderFile.rdbuf();

			shaderCode = shaderStream.str();
		}
		catch (const std::ifstream::failure& e)
		{
			XENGINE_ERROR("File open error: {}", e.what());
		}
		catch (const std::runtime_error& e)
		{
			XENGINE_ERROR("File read error: {}", e.what());
		}
		catch (const std::exception& e)
		{
			XENGINE_ERROR("Read File Error: {}", e.what());
		}
	}

	Shader::Shader(const char* vertexPath, const char* fragmentPath, const char* computePath)
	{

		mProgramId = glCreateProgram();
		

		int status = GL_FALSE;
		char errorLog[512]; 


		// Vertex Shader
		uint32_t vertexShaderId = glCreateShader(GL_VERTEX_SHADER); 
		{	
			readFile(vertexPath);
			const GLchar* glSource = shaderCode.c_str();
			glShaderSource(vertexShaderId, 1, &glSource, NULL); 
			glCompileShader(vertexShaderId); 
			glGetShaderiv(vertexShaderId, GL_COMPILE_STATUS, &status); 
			if (status != GL_TRUE)
			{
				glGetShaderInfoLog(vertexShaderId, sizeof(errorLog), NULL, errorLog); 
				XENGINE_ERROR("Vertex Shader compilation error: {}", errorLog);
				glDeleteShader(vertexShaderId);
				glDeleteProgram(mProgramId);
				return;
			}
			glAttachShader(mProgramId, vertexShaderId); 
		}

		// Fragment Shader
		uint32_t fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER); 
		if(status == GL_TRUE)
		{
			readFile(fragmentPath);
			const GLchar* glSource = shaderCode.c_str();
			glShaderSource(fragmentShaderId, 1, &glSource, NULL); 
			glCompileShader(fragmentShaderId); 
			glGetShaderiv(fragmentShaderId, GL_COMPILE_STATUS, &status); 
			if (status != GL_TRUE)
			{
				glGetShaderInfoLog(fragmentShaderId, sizeof(errorLog), NULL, errorLog); 
				XENGINE_ERROR("Fragment Shader compilation error: {}", errorLog);
				glDeleteShader(fragmentShaderId);
				glDeleteProgram(mProgramId);
				return;
			}
			glAttachShader(mProgramId, fragmentShaderId);
		}

		// Compute Shader
		uint32_t computerShaderId = glCreateShader(GL_COMPUTE_SHADER);
		{
			readFile(computePath);
			XENGINE_INFO("Shader code length: {}", shaderCode.length());
			XENGINE_INFO("Shader code: {}", shaderCode);
			const GLchar* glSource = shaderCode.c_str();
			glShaderSource(computerShaderId, 1, &glSource, NULL);
			glCompileShader(computerShaderId);
			glGetShaderiv(computerShaderId, GL_COMPILE_STATUS, &status);
			if (status != GL_TRUE)
			{
				glGetShaderInfoLog(computerShaderId, sizeof(errorLog), NULL, errorLog);
				XENGINE_ERROR("Compute Shader compilation error: {}", errorLog);
				glDeleteShader(computerShaderId);
				glDeleteProgram(mProgramId);
				return;
			}
			/*glAttachShader(mProgramId, computerShaderId);*/XENGINE_INFO("test");
		}

		XENGINE_ASSERT(status == GL_TRUE, "Error compiling shader");
		if (status == GL_TRUE)
		{
			glLinkProgram(mProgramId); 
			glValidateProgram(mProgramId); 
			glGetProgramiv(mProgramId, GL_LINK_STATUS, &status); 
			if (status != GL_TRUE)
			{
				glGetProgramInfoLog(mProgramId, sizeof(errorLog), NULL, errorLog); 
				XENGINE_ERROR("Shader link error: {}", errorLog); 
				glDeleteProgram(mProgramId); 
				mProgramId = -1;
			}
		}

		glDeleteShader(vertexShaderId); 
		glDeleteShader(fragmentShaderId); 
		glDeleteShader(computerShaderId);
	}

	Shader::~Shader()
	{
		glUseProgram(0); 
		glDeleteProgram(mProgramId); 
	}

	void Shader::bind()
	{
		glUseProgram(mProgramId); 
	}

	void Shader::unbind()
	{
		glUseProgram(0); 
	}
	void Shader::setUniformInt(const std::string& name, int val)
	{
		glUseProgram(mProgramId); 
		glUniform1i(static_cast<GLint>(getUniformLoctaion(name)), val); 
	}
	void Shader::setUniformFloat1(const std::string& name, float val1)
	{
		glUseProgram(mProgramId); 
		glUniform1f(static_cast<GLint>(getUniformLoctaion(name)), val1); 
	}
	void Shader::setUniformFloat2(const std::string& name, float val1, float val2)
	{
		glUseProgram(mProgramId); 
		glUniform2f(static_cast<GLint>(getUniformLoctaion(name)), val1, val2); 
	}
	void Shader::setUniformFloat3(const std::string& name, float val1, float val2, float val3)
	{
		glUseProgram(mProgramId); 
		glUniform3f(static_cast<GLint>(getUniformLoctaion(name)), val1, val2, val3); 
	}
	void Shader::setUniformFloat4(const std::string& name, float val1, float val2, float val3, float val4)
	{
		glUseProgram(mProgramId);
		glUniform4f(static_cast<GLint>(getUniformLoctaion(name)), val1, val2, val3, val4);
	}
	int Shader::getUniformLoctaion(const std::string& name)
	{
		auto it = mUniformLocations.find(name);
		if (it == mUniformLocations.end())
		{
			mUniformLocations[name] = glGetUniformLocation(mProgramId, name.c_str());
		}

		return mUniformLocations[name];
	}
}