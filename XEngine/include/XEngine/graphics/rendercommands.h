#pragma once

#include <memory>

namespace XEngine::graphics
{
	class VertexArray;
	class Shader;
	class Texture;
	class Framebuffer;
	class ComputeShader;

	namespace rendercommands
	{
		class RenderCommand
		{
		public:
			virtual void execute() = 0;
			virtual ~RenderCommand() {};
		};

		class RenderComputeShader : public RenderCommand
		{
		public:
			RenderComputeShader(/*std::weak_ptr<VertexArray> vertexArray
							, std::weak_ptr<Shader> shader
							, */std::weak_ptr<ComputeShader> computeShader)
				/*: mVertexArray(vertexArray)
				, mShader(shader)
				, */: mComputeShader(computeShader){}
			virtual void execute() override;

		private: 
			//std::weak_ptr<VertexArray> mVertexArray;
			//std::weak_ptr<Shader> mShader;
			std::weak_ptr<ComputeShader> mComputeShader;
		};


		class RenderVertexArrayTexture : public RenderCommand
		{
		public:
			RenderVertexArrayTexture(std::weak_ptr<VertexArray> vertexArray, std::weak_ptr<Texture> texture, std::weak_ptr<Shader> shader)
				: mVertexArray(vertexArray)
				, mTexture(texture)
				, mShader(shader)
			{
			}
			virtual void execute() override;

		private:
			std::weak_ptr<VertexArray> mVertexArray;
			std::weak_ptr<Shader> mShader;
			std::weak_ptr<Texture> mTexture;
		};

		class PushFramebuffer : public RenderCommand
		{
		public:
			PushFramebuffer(std::weak_ptr<Framebuffer> framebuffer) : mFramebuffer(framebuffer) {};
			virtual void execute() override;
		private:
			std::weak_ptr<Framebuffer> mFramebuffer;
		};

		class PopFramebuffer : public RenderCommand
		{
		public:
			PopFramebuffer() {}
			virtual void execute() override;
		};
	}
}