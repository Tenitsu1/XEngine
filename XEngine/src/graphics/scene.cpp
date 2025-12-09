// Engine Headers
#include "XEngine/graphics/scene.hpp"
#include "XEngine/log.h"
#include "XEngine/graphics/gltfLoader.h"
#include "XEngine/graphics/structs.hpp"
#include "XEngine/accelerators/bvh.h"
#include "XEngine/shaders/shader.h"
#include "XEngine/shaders/computeShader.h"
#include "XEngine/engine.h"

// External
#include <glad/glad.h>

Scene::Scene() {}

Scene::~Scene() {
    unload();
}

void Scene::CreateSSBO(GLuint& ssboID, GLsizeiptr size, const void* data) {
    if (ssboID != 0) glDeleteBuffers(1, &ssboID);
    glGenBuffers(1, &ssboID);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboID);
    glBufferData(GL_SHADER_STORAGE_BUFFER, size, data, GL_STATIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

bool Scene::load(const std::string& filepath) {
    if (!std::filesystem::exists(filepath)) {
        XENGINE_ERROR("Scene file not found: {}", filepath);
        return false;
    }

    XENGINE_INFO("Loading Scene: {}", filepath);

    // load model
    mModel = std::make_shared<XEngine::graphics::GLTFStaticMesh>(mtinyModel, filepath.c_str());

    auto& Mesh = mModel->getMesh();
    auto& packedTris = mModel->mPackedTriangles;
    std::vector<Material> materials = mModel->getMaterials();

    mTriangleCount = (int)Mesh.indices.size();
    mFilePath = filepath;

    XENGINE_TRACE("Building BVH...");
    uint64_t buildTimeStart = 0;
    XEngine::Engine::Instance().getWindow().getDeltaTime(buildTimeStart);
    auto bvhNodes = XEngine::BVH::buildBVH(Mesh, packedTris);
    float buildTime = XEngine::Engine::Instance().getWindow().getDeltaTime(buildTimeStart);
    XENGINE_TRACE("BVH Build Time: {:.4f} seconds", buildTime);
    XENGINE_TRACE("BVH Built. Nodes: {}", bvhNodes.size());

    // build SSBOs to upload shader
    if (mTriangleCount > 0)
    {
        CreateSSBO(mOBVHSSBO,        (uint32_t)bvhNodes.size() * sizeof(XEngine::BVH::BVHNode), bvhNodes.data());
        CreateSSBO(mVerticesSSBO,    (uint32_t)Mesh.vertices.size() * sizeof(glm::vec4),        Mesh.vertices.data());
        CreateSSBO(mIndicesSSBO,     (uint32_t)Mesh.indices.size() * sizeof(glm::ivec4),        Mesh.indices.data());
        CreateSSBO(mFaceNormalsSSBO, (uint32_t)Mesh.faceNormals.size() * sizeof(glm::vec4),     Mesh.faceNormals.data());
        CreateSSBO(mNormalsSSBO,     (uint32_t)Mesh.normals.size() * sizeof(glm::vec4),         Mesh.normals.data());

        if (!Mesh.texCoords.empty())
            CreateSSBO(mTexCoordsSSBO, (uint32_t)Mesh.texCoords.size() * sizeof(glm::vec2), Mesh.texCoords.data());

        if (!Mesh.materialIndices.empty())
            CreateSSBO(mMaterialIndicesSSBO, (uint32_t)Mesh.materialIndices.size() * sizeof(int), Mesh.materialIndices.data());

        if (!materials.empty())
            CreateSSBO(mMaterialDataSSBO, (uint32_t)materials.size() * sizeof(Material), materials.data());

        if (!packedTris.empty()) {
            CreateSSBO(mPackedTriSSBO, (uint32_t)packedTris.size() * sizeof(PackedTriangle), packedTris.data());
        }
        else {
            XENGINE_WARN("PackedTriangles is empty! Ray Tracing will fail.");
        }
    }

    mLoaded = true;
    XENGINE_INFO("Scene Loaded Successfully!");
    return true;

}

void Scene::unload() {
    if (!mLoaded) return;

    XENGINE_INFO("Unloading Scene...");


    // clean SSBO
    GLuint buffers[] = {
        mVerticesSSBO, mIndicesSSBO, mNormalsSSBO, mOBVHSSBO,
        mMaterialDataSSBO, mFaceNormalsSSBO, mTexCoordsSSBO,
        mMaterialIndicesSSBO, mPackedTriSSBO
    };

    for (auto& ssbo : buffers) {
        if (ssbo != 0) { glDeleteBuffers(1, &ssbo); ssbo = 0; }
    }

    // clean Model Data
    mtinyModel = tinygltf::Model(); // Reset tinygltf
    mModel.reset(); // Reset Mesh wrapper
    if (mModel) {
        unsigned int textureArrayID = mModel->getTextureArrayID();
        if (textureArrayID != 0) {
            glDeleteTextures(1, &textureArrayID);
        }
    }

    mTriangleCount = 0;
    mLoaded = false;
    mFilePath = "";
}

void Scene::DrawToGBuffer(std::shared_ptr<XEngine::Shader> shader) {
    if (!mLoaded || !mModel) return;
    mModel->drawWithShader(shader, mtinyModel);
}
void Scene::BindingToCompute(std::shared_ptr<XEngine::ComputeShader> computeShader) {
    if (!mLoaded) return;

    // Binding to Compute Shader
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, mOBVHSSBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, mVerticesSSBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, mIndicesSSBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, mFaceNormalsSSBO);

    if (mTexCoordsSSBO) glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, mTexCoordsSSBO);
    if (mMaterialIndicesSSBO) glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, mMaterialIndicesSSBO);
    if (mMaterialDataSSBO) glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, mMaterialDataSSBO);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 9, mNormalsSSBO);

    if (EnvTextureSSBO) computeShader->bindTexture(EnvTextureSSBO, 12);

    if (mPackedTriSSBO) glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 15, mPackedTriSSBO);

    if (mModel) {
        const auto& textures = mModel->getTextureArrayID();
        computeShader->bindTextureArray(textures, 20);
    }
}

void Scene::GetBoundsBox() const {
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
}


/*uint64_t buildTimeStart = 0;
Engine::Instance().getWindow().getDeltaTime(buildTimeStart);
XENGINE_TRACE("Starting to build BVH...");
auto bvhNodes = BVH::buildBVH(Mesh, packedTris);
float buildTime = Engine::Instance().getWindow().getDeltaTime(buildTimeStart);
XENGINE_TRACE("BVH Build Time: {:.4f} seconds", buildTime);
auto leafNode = BVH::getLeafNode(bvhNodes);


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
}*/
