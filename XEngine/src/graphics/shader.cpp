#include "graphics/shader.h"
#include "log.h"
#include "graphics/helper.h"

#include "glad/glad.h"

#include "external/glm/gtc/type_ptr.hpp"


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

	Shader::Shader(const char* vertexPath, const char* fragmentPath)
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

	void Shader::setUniformFloat2(const std::string& name, const glm::vec2& val)
	{
		setUniformFloat2(name, val.x, val.y);
	}

	void Shader::setUniformFloat3(const std::string& name, const glm::vec3& val)
	{
		setUniformFloat3(name, val.x, val.y, val.z);
	}

	void Shader::setUniformFloat4(const std::string& name, const glm::vec4& val)
	{
		setUniformFloat4(name, val.x, val.y, val.z, val.w);
	}


	void Shader::setUniformMat3(const std::string& name, const glm::mat3& mat)
	{
		glUseProgram(mProgramId);
		glUniformMatrix3fv(getUniformLoctaion(name), 1, GL_FALSE, glm::value_ptr(mat));
	}

	void Shader::setUniformMat4(const std::string& name, const glm::mat4& mat)
	{
		glUseProgram(mProgramId);
		glUniformMatrix4fv(getUniformLoctaion(name), 1, GL_FALSE, glm::value_ptr(mat));
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


	void ComputeShader::readFile(const char* shaderPath)
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


	ComputeShader::ComputeShader(const char* computePath, int width, int height)
		:mWidth(width), mHeight(height)
	{
		mProgramId = glCreateProgram();


		int status = GL_FALSE;
		char errorLog[512];


		// Compute Shader
		uint32_t computeShaderId = glCreateShader(GL_COMPUTE_SHADER);
		{
			readFile(computePath);
			const GLchar* glSource = shaderCode.c_str();
			glShaderSource(computeShaderId, 1, &glSource, NULL);
			glCompileShader(computeShaderId);
			glGetShaderiv(computeShaderId, GL_COMPILE_STATUS, &status);
			if (status != GL_TRUE)
			{
				glGetShaderInfoLog(computeShaderId, sizeof(errorLog), NULL, errorLog);
				XENGINE_ERROR("Compute Shader compilation error: {}", errorLog);
				glDeleteShader(computeShaderId);
				glDeleteProgram(mProgramId);
				return;
			}
			glAttachShader(mProgramId, computeShaderId);
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

		glDeleteShader(computeShaderId);

		glGenTextures(1, &mTexture);
		glBindTexture(GL_TEXTURE_2D, mTexture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, mWidth, mHeight, 0, GL_RGBA, GL_FLOAT, nullptr);
		glBindImageTexture(0, mTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	}

	void ComputeShader::bind()
	{
		glUseProgram(mProgramId);
	}

	void ComputeShader::unbind()
	{
		glUseProgram(0);
	}

	ComputeShader::~ComputeShader()
	{
		glUseProgram(0);
		glDeleteProgram(mProgramId);
	}

	void ComputeShader::createSSBO(uint32_t& ssbo, uint32_t size, const void* data, uint32_t binding)
	{
		glGenBuffers(1, &ssbo);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
		glBufferData(GL_SHADER_STORAGE_BUFFER, size, data, GL_STATIC_DRAW);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, ssbo);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	}

	void ComputeShader::DispatchCompute()
	{
		glBindImageTexture(0, mTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
		glDispatchCompute((mWidth + 15) / 16, (mHeight + 15) / 16, 1);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	}
}