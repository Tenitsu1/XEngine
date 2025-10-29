#pragma once

#include <memory>

namespace XEngine::graphics
{
	class Mesh;
	class Shader;
	class Texture;
	class Framebuffer;

	namespace rendercommands
	{
		class RenderCommand
		{
		public:
			virtual void execute() = 0;
			virtual ~RenderCommand() {};
		};

		class RenderMesh : public RenderCommand
		{
		public:
			RenderMesh(std::weak_ptr<Mesh> mesh, std::weak_ptr<Shader> shader)
				:mMesh(mesh)
				, mShader(shader)
			{}
			virtual void execute() override;

		private: 
			std::weak_ptr<Mesh> mMesh;
			std::weak_ptr<Shader> mShader;
		};


		class RenderMeshTexture : public RenderCommand
		{
		public:
			RenderMeshTexture(std::weak_ptr<Mesh> mesh, std::weak_ptr<Texture> texture ,std::weak_ptr<Shader> shader)
				: mMesh(mesh)
				, mTexture(texture)
				, mShader(shader)
			{
			}
			virtual void execute() override;

		private:
			std::weak_ptr<Mesh> mMesh;
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