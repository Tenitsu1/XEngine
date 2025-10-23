#pragma once

#include <memory>

namespace XEngine::graphics
{
	class Mesh;
	class Shader;

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
			virtual void execute();

		private:
			std::weak_ptr<Mesh> mMesh;
			std::weak_ptr<Shader> mShader;
		};
	}
}