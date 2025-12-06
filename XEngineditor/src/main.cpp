#include "XEngine/engine.h"
#include "XEngine/app.h"
#include "XEngine/log.h"


#include "XEngine/shaders/computeShader.h"
#include "XEngine/shaders/shader.h"

#include "XEngine/graphics/gltfLoader.h"
#include "XEngine/graphics/structs.hpp"
#include "XEngine/graphics/camera.hpp"
#include "XEngine/graphics/cameraController.hpp"
#include "XEngine/graphics/Gbuffer.h"

#include "XEngine/input/mouse.h"
#include "XEngine/input/keyboard.h"

#include "XEngine/accelerators/bvh.h"

#include "external/imgui/imgui.h"
#include "external/glm/glm.hpp"
#include "external/glm/gtc/matrix_transform.hpp"
#include "external/tinygltf/tiny_gltf.h"

#include <string>
#include <filesystem>

using namespace XEngine;

class Editor : public XEngine::App
{

private:

	// Shader
	std::shared_ptr<ComputeShader> mComputeShader;
	std::shared_ptr<Shader> mShader;

	// Model
	tinygltf::Model mtinyModel;
	std::shared_ptr<graphics::GLTFStaticMesh> mModel;

	// Gbuffer
	std::shared_ptr<graphics::GBuffer> mGBuffer;
	std::shared_ptr<Shader> mGBufferShader;

	// Camera && Controller
	Camera camera;
	std::unique_ptr<CameraController> mCameraController;
	float xkeyOffset = 0.f;
	float ykeyOffset = 0.f;
	float zkeyOffset = 5.f;
	bool cameraUpdated = false;
	bool GuiCameraChanged = false;

	// SSBO
	GLuint mVerticesSSBO = 0; // vertex  SSBO-ID
	GLuint mIndicesSSBO = 0;  // indices SSBO-ID
	GLuint mNormalsSSBO = 0;  // normal  SSBO-ID
	GLuint mOBVHSSBO = 0;     // OBVH    SSBO-ID
	GLuint mMaterialDataSSBO = 0;
	GLuint mFaceNormalsSSBO = 0;
	GLuint mTexCoordsSSBO = 0;
	GLuint mMaterialIndicesSSBO = 0;
	GLuint mMatToTexMapSSBO = 0;
	GLuint mPackedTriSSBO = 0;

	GLuint mScreenTexture = 0;
	int mCurrentFrame = 0;

	uint64_t nowTime = 0;
	float deltaTime = 0;
	int triangleCount = 0; 

	int samples_per_pixel = 1;
	int max_depth = 5;

	float lastX = 640, lastY = 450;
	bool useOBVH = true;

public:

	core::WindowProperties getWindowProperties()
	{
		core::WindowProperties props;
		props.title = "RayTracing Project";
		props.width = 1600;
		props.height = 900;
		props.imguiProps.isViewportEnable = true;
		props.imguiProps.isDockingEnable = true;
		return props;
	}

	void initialize() override
	{
		int width = getWindowProperties().width;
		int height = getWindowProperties().height;

		/*mModel = std::make_shared<graphics::GLTFStaticMesh>(mtinyModel, "models\\japanese_classroom\\sceneWithLight.gltf");*/
		mModel = std::make_shared<graphics::GLTFStaticMesh>(mtinyModel, "models\\japanese_classroom\\sceneWithLight.gltf");
		auto& Mesh = mModel->getMesh();

		std::vector<Material> materials = mModel->getMaterials();
		auto& packedTris = mModel->mPackedTriangles;

		mGBuffer = std::make_shared<graphics::GBuffer>();
		if (!mGBuffer->initialize(width, height)) XENGINE_ERROR("Failed to initialize GBuffer!");
		mGBufferShader = std::make_shared<Shader>("shaders/gbuffer.vert", "shaders/gbuffer.frag");

		mShader = std::make_shared<Shader>("shaders\\default.vert", "shaders\\default.frag");
		mShader->createTexture(width, height);
		mShader->setFBO();
		mShader->setVAO();
		mShader->setVBO();
		float quadVertices[] = {
			// positions   // texCoords
			-1.0f, -1.0f,   0.0f, 0.0f,
			 1.0f, -1.0f,   1.0f, 0.0f,
			 1.0f,  1.0f,   1.0f, 1.0f,
			-1.0f,  1.0f,   0.0f, 1.0f
		};
		unsigned int quadIndices[] = { 0, 1, 2, 0, 2, 3 };
		mShader->setEBO(quadIndices, sizeof(quadIndices));
		mShader->bind(quadVertices, 4, 4);

		mComputeShader = std::make_shared<ComputeShader>("shaders\\test.glsl", width, height);
		// mComputeShader->chackBindLimit();
		mScreenTexture = mComputeShader->createTexture(width, height);

		uint64_t buildTimeStart = 0;
		Engine::Instance().getWindow().getDeltaTime(buildTimeStart);
		XENGINE_TRACE("Starting to build BVH...");
		auto bvhNodes = BVH::buildBVH(Mesh, packedTris);
		float buildTime = Engine::Instance().getWindow().getDeltaTime(buildTimeStart);
		XENGINE_TRACE("BVH Build Time: {:.4f} seconds", buildTime);
		auto leafNode = BVH::getLeafNode(bvhNodes);


		triangleCount = (int)Mesh.indices.size();
		if (triangleCount > 0)
		{
			mComputeShader->createSSBO(mOBVHSSBO, (uint32_t)bvhNodes.size() * sizeof(BVH::BVHNode), bvhNodes.data(), 2);
			mComputeShader->createSSBO(mVerticesSSBO, (uint32_t)Mesh.vertices.size() * sizeof(glm::vec4), Mesh.vertices.data(), 3);
			mComputeShader->createSSBO(mIndicesSSBO, (uint32_t)Mesh.indices.size() * sizeof(glm::ivec4), Mesh.indices.data(), 4);
			mComputeShader->createSSBO(mFaceNormalsSSBO, (uint32_t)Mesh.faceNormals.size() * sizeof(glm::vec4), Mesh.faceNormals.data(), 5);
			mComputeShader->createSSBO(mNormalsSSBO, (uint32_t)Mesh.normals.size() * sizeof(glm::vec4), Mesh.normals.data(), 9);

			if (!Mesh.texCoords.empty())
			{
				mComputeShader->createSSBO(mTexCoordsSSBO, (uint32_t)Mesh.texCoords.size() * sizeof(glm::vec2), Mesh.texCoords.data(), 6);
			}
			else
			{
				XENGINE_WARN("Model has no texture coordinates!");
			}

			// Material Indices (Binding 7)
			if (!Mesh.materialIndices.empty())
			{
				mComputeShader->createSSBO(mMaterialIndicesSSBO,
					(uint32_t)Mesh.materialIndices.size() * sizeof(int),
					Mesh.materialIndices.data(), 7);
			}

			// Material Data SSBO (Binding 8)
			if (!materials.empty())
			{
				mComputeShader->createSSBO(mMaterialDataSSBO,
					(uint32_t)materials.size() * sizeof(Material),
					materials.data(), 8);
			}

			const auto& textures = mModel->getTextures();
			int limit = std::min((int)textures.size(), 28);
			for (int i = 0; i < limit; ++i) {
				// 假設 Compute Shader 裡 u_textures 改成了 binding = 20
				mComputeShader->bindTexture(textures[i], 20 + i);
			}

			if (!packedTris.empty()) {
				mComputeShader->createSSBO(mPackedTriSSBO,
					(uint32_t)packedTris.size() * sizeof(PackedTriangle),
					packedTris.data(), 15);

				XENGINE_TRACE("PackedTriangle SSBO created, size: {}", packedTris.size());
			}

			{
				XENGINE_TRACE("Vertices SSBO created, size: {}, count: {}",
					(uint32_t)Mesh.vertices.size() * sizeof(glm::vec4), Mesh.vertices.size());
				XENGINE_TRACE("Indices SSBO created, size: {}, count: {}",
					(uint32_t)Mesh.indices.size() * sizeof(glm::ivec4), Mesh.indices.size());
				XENGINE_TRACE("Normals SSBO created, size: {}, count: {}",
					(uint32_t)Mesh.normals.size() * sizeof(glm::vec4), Mesh.normals.size());
				XENGINE_TRACE("FaceNormals SSBO created, size: {}, count: {}",
					(uint32_t)Mesh.faceNormals.size() * sizeof(glm::vec4), Mesh.faceNormals.size());
				XENGINE_TRACE("TexCoords SSBO created, size: {}, count: {}",
					(uint32_t)Mesh.texCoords.size() * sizeof(glm::vec2), Mesh.texCoords.size());
				XENGINE_TRACE("MaterialIndices SSBO created, size: {}, count: {}",
					(uint32_t)Mesh.materialIndices.size() * sizeof(int), Mesh.materialIndices.size());
				XENGINE_TRACE("MaterialData SSBO created, size: {}, count: {}",
					(uint32_t)materials.size() * sizeof(Material), materials.size());
				XENGINE_TRACE("BVH Nodes SSBO created, size: {}, count: {}",
					(uint32_t)bvhNodes.size() * sizeof(BVH::BVHNode), bvhNodes.size());
				XENGINE_TRACE("{} Leafs (min, median, max, mode, avg)", leafNode.count);
				XENGINE_TRACE("depth    = ({}, {:.1f}, {}, {}, {:.2f})",
					leafNode.depth.minValue, leafNode.depth.medianValue, leafNode.depth.maxValue, leafNode.depth.modeValue, leafNode.depth.averageValue);
				XENGINE_TRACE("triangle = ({}, {:.1f}, {}, {}, {:.2f})",
					leafNode.triangleCount.minValue, leafNode.triangleCount.medianValue, leafNode.triangleCount.maxValue, leafNode.triangleCount.modeValue, leafNode.triangleCount.averageValue);
			}
		}

		// --- 印出包圍盒日誌 ---
		glm::vec3 boundsMin = mModel->getBoundsMin();
		glm::vec3 boundsMax = mModel->getBoundsMax();
		glm::vec3 center = (boundsMin + boundsMax) * 0.5f;
		glm::vec3 size = boundsMax - boundsMin;

		XENGINE_TRACE("=========================================");
		XENGINE_TRACE("Model Bounding Box Info:");
		XENGINE_TRACE("  Min: ({:.2f}, {:.2f}, {:.2f})", boundsMin.x, boundsMin.y, boundsMin.z);
		XENGINE_TRACE("  Max: ({:.2f}, {:.2f}, {:.2f})", boundsMax.x, boundsMax.y, boundsMax.z);
		XENGINE_TRACE("  Center: ({:.2f}, {:.2f}, {:.2f})", center.x, center.y, center.z);
		XENGINE_TRACE("  Size: ({:.2f}, {:.2f}, {:.2f})", size.x, size.y, size.z);
		XENGINE_TRACE("=========================================");



		camera = Camera(glm::vec3(8.f, 4.0f, 0.2f));
		camera.Yaw = -180.0f;
		camera.Pitch = 0.0f;
		camera.MovementSpeed = 5.0f; 
		camera.ProcessMouseMovement(0, 0);

		mCameraController = std::make_unique<CameraController>(camera);
		mCameraController->SetSpeed(10.0f);

	}
	void shutdown() override
	{
		
	}
	void update() override
	{
		auto windowSize = Engine::Instance().getWindow().getWindowSize();

		// Calculate the delta time
		deltaTime = Engine::Instance().getWindow().getDeltaTime(nowTime);

		float timeStep = deltaTime * .1f;


		// Camera Updata
		cameraUpdated = mCameraController->OnUpdate(timeStep) || GuiCameraChanged;
		GuiCameraChanged = false;
	}

	void render() override
	{
		int width = getWindowProperties().width;
		int height = getWindowProperties().height;

		glm::mat4 view = camera.GetViewMatrix();
		glm::mat4 projection = camera.GetProjectionMatrix((float)width, (float)height);

		// =============================================================
		// Phase 1: Geometry Pass (Rasterization -> GBuffer)
		// =============================================================
		{
			mGBuffer->bindForWriting();

			mGBufferShader->bind();

			mGBufferShader->setUniformMat4("view", view);
			mGBufferShader->setUniformMat4("projection", projection);
			mGBufferShader->setUniformMat4("model", glm::mat4(1.0f));

			mGBufferShader->setUniformInt("texture_baseColor", 0);
			mGBufferShader->setUniformInt("texture_metallicRoughness", 1);
			mGBufferShader->setUniformInt("texture_normal", 2);
			mGBufferShader->setUniformInt("texture_emissive", 3);

			mModel->drawWithShader(mGBufferShader, mtinyModel);

			mGBufferShader->unbind();
		}

		// =============================================================
		// Phase 2: Compute Pass (Ray Tracing)
		// =============================================================
		{
			mComputeShader->bind();


			mGBuffer->bindForReading(10);

			// 設定 Uniforms
			CameraData cameraShaderData = camera.GetShaderData();
			glm::mat4 invViewProj = glm::inverse(projection * view);

			mComputeShader->setUniformFloat2("u_resolution", (float)width, (float)height);
			mComputeShader->setUniformFloat1("u_time", (float)nowTime / 1000.0f);
			mComputeShader->setUniformCamera("camera", cameraShaderData);
			mComputeShader->setUniformBool("cameraUpdated", cameraUpdated);
			mComputeShader->setUniformMat4("invViewProj", invViewProj);
			mComputeShader->setUniformInt("SAMPLES_PER_PIXEL", samples_per_pixel);
			mComputeShader->setUniformInt("MAX_DEPTH", max_depth);
			mComputeShader->setUniformBool("useOBVH", useOBVH);
 

			mComputeShader->DispatchCompute(mScreenTexture);
		}

		// =============================================================
		// Phase 3: Post-Processing Pass (Display to Screen/ImGui FBO)
		// =============================================================
		{
			mShader->bind();
			mShader->bindTexture(mScreenTexture, 0, "screenTexture");
			mShader->draw(width, height);
		}
	}

	void imguiRender() override
	{
		ImGui::DockSpaceOverViewport(ImGui::GetMainViewport()->ID);

		ImGuiIO& io = ImGui::GetIO();
		ImGui::ShowDemoWindow();


		if (ImGui::Begin("Settings"))
		{
			ImGui::Text("Render Stats:");
			ImGui::Text("Average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
			ImGui::Text("%d vertices,\n%d indices (%d triangles)",
				io.MetricsRenderVertices,
				io.MetricsRenderIndices,
				triangleCount);

			ImGui::Separator();
			GuiCameraChanged |= ImGui::DragInt("Samples", &samples_per_pixel, 1, 1, 100);
			GuiCameraChanged |= ImGui::DragInt("Max Bounces", &max_depth, 1, 1, 20);
			ImGui::Checkbox("Use OBVH", &useOBVH);

			ImGui::Separator();
			GuiCameraChanged |= ImGui::DragFloat3("Camera Position", &camera.Position.x, 0.1f);
			GuiCameraChanged |= ImGui::DragFloat("Camera Zoom", &camera.Zoom, 45, 1, 90);

			float speed = mCameraController->GetSpeed();
			if (ImGui::DragFloat("Cam Speed", &speed, 0.1f)) mCameraController->SetSpeed(speed);
		}
		ImGui::End();



		if (ImGui::Begin("Scene"))
		{
			if (ImGui::IsItemHovered() || ImGui::IsWindowHovered())
			{
				ImGui::SetNextFrameWantCaptureMouse(false);
			}

			ImGui::Image(
					(void*)(intptr_t)mShader->getTexture(),
					{1280, 720 },
					ImVec2(0, 1),
					ImVec2(1, 0));
		}
		ImGui::End();

		if (ImGui::Begin("GBuffer Debug"))
		{
			ImGui::Text("Position");
			float w = 320, h = 180;
			ImGui::Image((void*)(intptr_t)mGBuffer->getTexture(graphics::GBuffer::GBUFFER_POSITION), { w, h }, { 0,1 }, { 1,0 });
		}
		ImGui::End();

		if (ImGui::Begin("GBuffer Debug"))
		{
			ImGui::Text("Normal");
			float w = 320, h = 180;
			ImGui::Image((void*)(intptr_t)mGBuffer->getTexture(graphics::GBuffer::GBUFFER_NORMAL), { w, h }, { 0,1 }, { 1,0 });
		}
		ImGui::End();

		if (ImGui::Begin("GBuffer Debug"))
		{
			ImGui::Text("Emission");
			float w = 320, h = 180;
			ImGui::Image((void*)(intptr_t)mGBuffer->getTexture(graphics::GBuffer::GBUFFER_EMISSION), { w, h }, { 0,1 }, { 1,0 });
		}
		ImGui::End();


		ImGui::Begin("My Window");

		if (ImGui::Button("Screenshot")) {
			int num = 0;
			std::string path;
			while (true)
			{
			// "image\\output.png"
				
				path = "image\\output_" + std::to_string(num) + ".png";
				if (!std::filesystem::exists(path)) break;
				num++;
			}
			mShader->exportPNG(path.c_str(), getWindowProperties().width, getWindowProperties().height);
		}

		ImGui::End();
	}

};


static XEngine::App* createApp()
{
	return new Editor();
}

//XEngine::App* createApp();

int main(int argc, char const *argv[])
{
	XEngine::App* app = createApp();
	XEngine::Engine::Instance().run(app);
	
	delete app;
	return 0;
}