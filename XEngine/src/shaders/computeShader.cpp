#include "shaders/ComputeShader.h"
#include "log.h"
#include "glad/glad.h"
#include "graphics/structs.hpp"
#include "graphics/camera.hpp"
#include "external/stb/stb_image.h"

#include "external/glm/gtc/type_ptr.hpp"


namespace XEngine
{
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

	void ComputeShader::DispatchCompute(uint32_t outputTexture)
	{
		glUseProgram(mProgramId);
		glBindImageTexture(0, outputTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
		glDispatchCompute((mWidth + 15) / 16, (mHeight + 15) / 16, 1);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	}

	int ComputeShader::getUniformLoctaion(const std::string& name)
	{
		auto it = mUniformLocations.find(name);
		if (it == mUniformLocations.end())
		{
			mUniformLocations[name] = glGetUniformLocation(mProgramId, name.c_str());
		}

		return mUniformLocations[name];
	}

	void ComputeShader::setUniformInt(const std::string& name, int val)
	{
		glUniform1i(static_cast<GLint>(getUniformLoctaion(name)), val);
	}

	void ComputeShader::setUniformBool(const std::string& name, bool val) {
		glUniform1i(static_cast<GLint>(getUniformLoctaion(name)), static_cast<int>(val));
	}

	void ComputeShader::setUniformFloat1(const std::string& name, float val1)
	{
		glUniform1f(static_cast<GLint>(getUniformLoctaion(name)), val1);
	}

	void ComputeShader::setUniformFloat2(const std::string& name, float val1, float val2)
	{
		glUniform2f(static_cast<GLint>(getUniformLoctaion(name)), val1, val2);
	}

	void ComputeShader::setUniformFloat3(const std::string& name, float val1, float val2, float val3)
	{
		glUniform3f(static_cast<GLint>(getUniformLoctaion(name)), val1, val2, val3);
	}

	void ComputeShader::setUniformFloat4(const std::string& name, float val1, float val2, float val3, float val4)
	{
		glUniform4f(static_cast<GLint>(getUniformLoctaion(name)), val1, val2, val3, val4);
	}

	void ComputeShader::setUniformFloat2(const std::string& name, const glm::vec2& val)
	{
		setUniformFloat2(name, val.x, val.y);
	}

	void ComputeShader::setUniformFloat3(const std::string& name, const glm::vec3& val)
	{
		setUniformFloat3(name, val.x, val.y, val.z);
	}

	void ComputeShader::setUniformFloat4(const std::string& name, const glm::vec4& val)
	{
		setUniformFloat4(name, val.x, val.y, val.z, val.w);
	}

	void ComputeShader::setUniformMat3(const std::string& name, const glm::mat3& mat)
	{
		glUniformMatrix3fv(getUniformLoctaion(name), 1, GL_FALSE, glm::value_ptr(mat));
	}

	void ComputeShader::setUniformMat4(const std::string& name, const glm::mat4& mat)
	{
		glUniformMatrix4fv(getUniformLoctaion(name), 1, GL_FALSE, glm::value_ptr(mat));
	}

	void ComputeShader::setUniformCamera(const std::string& name, const CameraData& camera)
	{
		setUniformFloat3(name + ".position", camera.position);
		setUniformFloat3(name + ".lookat", camera.lookat);
		setUniformFloat3(name + ".up", camera.up);
		setUniformFloat1(name + ".fov", camera.fov);
	}

	uint32_t ComputeShader::createTexture(int width, int height) {
		uint32_t tex;
		glGenTextures(1, &tex);
		glBindTexture(GL_TEXTURE_2D, tex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
		glBindTexture(GL_TEXTURE_2D, 0);
		return tex;
	}

	void ComputeShader::bindImageTexture(uint32_t textureID, int binding) {
		glBindImageTexture(binding, textureID, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
	}

	void ComputeShader::bindTexture(uint32_t textureID, int textureUnit)
	{
		glActiveTexture(GL_TEXTURE0 + textureUnit);
		glBindTexture(GL_TEXTURE_2D, textureID);
	}

	void ComputeShader::bindTextureArray(uint32_t textureID, int textureUnit)
	{
		glActiveTexture(GL_TEXTURE0 + textureUnit);
		glBindTexture(GL_TEXTURE_2D_ARRAY, textureID);
	}

	uint32_t ComputeShader::bindHDRIsTexture(const char* path)
	{
		stbi_set_flip_vertically_on_load(true);
		int width, height, nrComponents;
		float* data = stbi_loadf(path, &width, &height, &nrComponents, 0);

		uint32_t mHDRiTexture;

		if (data) {
			glCreateTextures(GL_TEXTURE_2D, 1, &mHDRiTexture);

			// 使用 GL_RGB32F 或 GL_RGB16F 來儲存高動態範圍數據
			glTextureStorage2D(mHDRiTexture, 1, GL_RGB32F, width, height);
			glTextureSubImage2D(mHDRiTexture, 0, 0, 0, width, height, GL_RGB, GL_FLOAT, data);

			// 設定環繞與過濾模式
			glTextureParameteri(mHDRiTexture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // 防止邊緣縫隙
			glTextureParameteri(mHDRiTexture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTextureParameteri(mHDRiTexture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTextureParameteri(mHDRiTexture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			stbi_image_free(data);
			XENGINE_TRACE("HDR Texture loaded: {}", path);
		}
		else {
			XENGINE_ERROR("Failed to load HDR image: {}", path);
			mHDRiTexture = 0;
		}

		return mHDRiTexture;
	}

	void ComputeShader::createDebugSSBO(uint32_t binding)
	{
		// 如果已經創建過，先刪除舊的
		if (mDebugSSBO != 0) {
			glDeleteBuffers(1, &mDebugSSBO);
		}

		mDebugSSBOSize = sizeof(DebugData);

		glGenBuffers(1, &mDebugSSBO);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, mDebugSSBO);
		// GL_DYNAMIC_READ 提示驅動程式，我們將會從 GPU 讀取這個 buffer
		glBufferData(GL_SHADER_STORAGE_BUFFER, mDebugSSBOSize, nullptr, GL_DYNAMIC_READ);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, mDebugSSBO);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); // 解綁

		XENGINE_TRACE("Debug SSBO created with size {} bytes, bound to {}", mDebugSSBOSize, binding);
	}

	std::optional<DebugData> ComputeShader::readDebugData()
	{
		if (mDebugSSBO == 0) {
			return std::nullopt; // 如果 SSBO 沒有被創建，回傳空
		}

		DebugData result;

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, mDebugSSBO);

		// 使用 glGetBufferSubData 是另一種安全的方式，比 glMapBuffer 更簡單
		glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, mDebugSSBOSize, &result);

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); // 解綁

		return result;
	}

	void ComputeShader::chackBindLimit()
	{
		GLint maxComputeTexUnits;
		glGetIntegerv(GL_MAX_COMPUTE_TEXTURE_IMAGE_UNITS, &maxComputeTexUnits);
		XENGINE_TRACE("Max Compute Texture Units: {}", maxComputeTexUnits);

		GLint maxCombinedTexUnits;
		glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxCombinedTexUnits);
		XENGINE_TRACE("Max Combined Texture Units: {}", maxCombinedTexUnits);
	}
}