#include "graphics/shader.h"
#include "log.h"

#include "glad/glad.h"

//std::string get_file_contents(const std::string& filename)
//{
//	std::ifstream in(filename, std::ios::binary);
//	if (in)
//	{
//		std::string contents;
//		in.seekg(0, std::ios::end);
//		contents.resize(in.tellg());
//		in.seekg(0, std::ios::beg);
//		in.read(&contents[0], contents.size());
//		in.close();
//		return(contents);
//	}
//	throw(errno);
//}

namespace XEngine::graphics
{
	void Shader::readFile(const char* vertexPath, const char* fragmentPath)
	{
		std::ifstream vertexFile;
		std::ifstream fragmentFile;
		vertexFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		fragmentFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

		try
		{
			vertexFile.open(vertexPath);
			if (!vertexFile.is_open()) {
				throw std::runtime_error("Could not open vertex shader file.");
			}

			fragmentFile.open(fragmentPath);
			if (!fragmentFile.is_open()) {
				throw std::runtime_error("Could not open fragment shader file.");
			}

			std::stringstream vertexStream, fragmentStream;
			vertexStream << vertexFile.rdbuf();
			fragmentStream << fragmentFile.rdbuf();

			vertexCode = vertexStream.str();
			fragmentCode = fragmentStream.str();
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
		
		readFile(vertexPath, fragmentPath);
		
		
		mProgramId = glCreateProgram();
		

		int status = GL_FALSE;
		char errorLog[512]; 


		// Vertex Shader
		uint32_t vertexShaderId = glCreateShader(GL_VERTEX_SHADER); 
		{	
			const GLchar* glSource = vertexCode.c_str();
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
			const GLchar* glSource = fragmentCode.c_str();
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