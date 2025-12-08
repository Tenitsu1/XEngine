#pragma once
#include <string>
#include <fstream>
#include <sstream>
#include <optional>

#include "external/glm/glm.hpp"
#include <unordered_map>

struct CameraData;

namespace XEngine
{

	struct DebugData {
		// We will store values from the first triangle test
		glm::vec3 dbg_rayOrigin;
		float dbg_det;
		glm::vec3 dbg_rayDir;
		float dbg_t;
		glm::vec3 dbg_v0;
		float dbg_u;
		glm::vec3 dbg_v1;
		float dbg_v;
		glm::vec3 dbg_v2;
		float padding; // for alignment
	};


	class ComputeShader
	{
		public:
			ComputeShader(const char* computePath, int width, int height);
			~ComputeShader();

			void createSSBO(uint32_t& ssbo, uint32_t size, const void* data, uint32_t binding);
			void DispatchCompute(uint32_t outputTexture);

			void bind();
			void unbind();

			void setUniformInt(const std::string& name, int val);
			void setUniformBool(const std::string& name, bool val);
			void setUniformFloat1(const std::string& name, float val1);
			void setUniformFloat2(const std::string& name, float val1, float val2);
			void setUniformFloat3(const std::string& name, float val1, float val2, float val3);
			void setUniformFloat4(const std::string& name, float val1, float val2, float val3, float val4);

			void setUniformFloat2(const std::string& name, const glm::vec2& val);
			void setUniformFloat3(const std::string& name, const glm::vec3& val);
			void setUniformFloat4(const std::string& name, const glm::vec4& val);

			void setUniformMat3(const std::string& name, const glm::mat3& mat);
			void setUniformMat4(const std::string& name, const glm::mat4& mat);

			void setUniformCamera(const std::string& name, const CameraData& camera);

			uint32_t createTexture(int width, int height);
			void bindImageTexture(uint32_t textureID, int binding);
			void bindTexture(uint32_t textureID, int textureUnit);
			void bindTextureArray(uint32_t textureID, int textureUnit);
			uint32_t bindHDRIsTexture(const char* path);

			void createDebugSSBO(uint32_t binding);

			std::optional<DebugData> readDebugData();

			void chackBindLimit();

			void clearTexture(unsigned int textureID, int width, int height);

		private:
			uint32_t mDebugSSBO = 0; // 除錯 SSBO 的 ID
			uint32_t mDebugSSBOSize = 0; // 儲存 SSBO 的大小

		private:
			int getUniformLoctaion(const std::string& name);
			void readFile(const char* shaderPath);

		private:
			std::unordered_map<std::string, int> mUniformLocations;
			std::string shaderCode;
			uint32_t mProgramId;
			uint32_t mWidth;
			uint32_t mHeight;

	};
}


