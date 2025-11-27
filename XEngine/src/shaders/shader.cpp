#include "shaders/shader.h"
#include "log.h"

#include "glad/glad.h"

#include "external/glm/gtc/type_ptr.hpp"
#include "external/stb/stb_image_write.h"


namespace XEngine
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
		if (status == GL_TRUE)
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

	void Shader::bind(const float* vertexArray, uint32_t vertexCount, uint32_t dimensions) 
	{
		glBufferData(GL_ARRAY_BUFFER, vertexCount * dimensions * sizeof(float), vertexArray, GL_STATIC_DRAW);
		glVertexAttribPointer(0, dimensions, GL_FLOAT, GL_FALSE, dimensions * sizeof(float), 0); 
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
		glEnableVertexAttribArray(1);
	}

	void Shader::unbind()
	{
		glUseProgram(0);
	}

	void Shader::draw(int width, int height)
	{
		glUseProgram(mProgramId);

		// 綁定 FBO，讓 fragment shader 輸出到 mTexture
		glBindFramebuffer(GL_FRAMEBUFFER, FBO);
		glViewport(0, 0, width, height);

		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		// 解綁 FBO，回到預設 framebuffer
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void Shader::setVAO()
	{
		glGenVertexArrays(1, &VAO);
		glBindVertexArray(VAO);
	}

	void Shader::setVBO()
	{
		glGenBuffers(1, &VBO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
	}

	void Shader::setEBO(const void* data, size_t size)
	{
		glGenBuffers(1, &EBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
	}

	void Shader::setFBO()
	{
		glGenFramebuffers(1, &FBO);
		glBindFramebuffer(GL_FRAMEBUFFER, FBO);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mTexture, 0);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void Shader::setUniformInt(const std::string& name, int val)
	{
		glUniform1i(static_cast<GLint>(getUniformLoctaion(name)), val);
	}

	void Shader::setUniformBool(const std::string& name, bool val)
	{
		glUniform1i(static_cast<GLint>(getUniformLoctaion(name)), static_cast<int>(val));
	}

	void Shader::setUniformFloat1(const std::string& name, float val1)
	{
		glUniform1f(static_cast<GLint>(getUniformLoctaion(name)), val1);
	}

	void Shader::setUniformFloat2(const std::string& name, float val1, float val2)
	{
		glUniform2f(static_cast<GLint>(getUniformLoctaion(name)), val1, val2);
	}

	void Shader::setUniformFloat3(const std::string& name, float val1, float val2, float val3)
	{
		glUniform3f(static_cast<GLint>(getUniformLoctaion(name)), val1, val2, val3);
	}

	void Shader::setUniformFloat4(const std::string& name, float val1, float val2, float val3, float val4)
	{
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
		glUniformMatrix3fv(getUniformLoctaion(name), 1, GL_FALSE, glm::value_ptr(mat));
	}

	void Shader::setUniformMat4(const std::string& name, const glm::mat4& mat)
	{
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

	void Shader::bindTexture(uint32_t textureID, uint32_t textureUnit, const std::string& uniformName)
	{
		glActiveTexture(GL_TEXTURE0 + textureUnit);
		glBindTexture(GL_TEXTURE_2D, textureID);
		glUniform1i(glGetUniformLocation(mProgramId, uniformName.c_str()), textureUnit);
	}

	void Shader::createTexture(int width, int height)
	{
		glGenTextures(1, &mTexture);
		glBindTexture(GL_TEXTURE_2D, mTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}

	void Shader::exportPNG(const std::string& path, int width, int height)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, FBO);
		glPixelStorei(GL_PACK_ALIGNMENT, 1);

		std::vector<unsigned char> pixels(width * height * 4);

		glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		stbi_flip_vertically_on_write(true);

		int result = stbi_write_png(path.c_str(), width, height, 4, pixels.data(), width * 4);

		if (result) {
			XENGINE_INFO("Successfully saved PNG to: {}", path);
		}
		else {
			XENGINE_ERROR("Failed to save PNG to: {}", path);
		}
	}
}