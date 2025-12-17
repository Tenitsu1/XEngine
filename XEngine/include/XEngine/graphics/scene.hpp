#pragma once

#include <string>
#include <vector>
#include <memory>
#include <filesystem>

// External
#include "external/tinygltf/tiny_gltf.h"
#include "external/glm/glm.hpp"

typedef unsigned int GLuint;
typedef signed   long long int GLsizeiptr;

namespace XEngine
{
    class Shader;
    class ComputeShader;
}
namespace XEngine::graphics
{
    class GLTFStaticMesh;
}


class Scene
{
    public:
    	Scene();
    	~Scene();
    
    	bool load(const std::string& filePath);
    	void unload();
    
    	void DrawToGBuffer(std::shared_ptr<XEngine::Shader> shader);
    	void BindingToCompute(std::shared_ptr<XEngine::ComputeShader> computeShader);
    
    	// Getters
    	bool isLoaded() const { return mLoaded; }
    	int getTriangleCount() const { return mTriangleCount; }
        int getVertexCount() const { return mVertexCount; }
        int getIndexCount() const { return mIndexCount; }
    	std::string getFilePath() const { return mFilePath; }
    
    	void GetBoundsBox() const;
    
    private:
    	void CreateSSBO(GLuint& ssboID, GLsizeiptr size, const void* data);
    
    private:
        bool mLoaded = false;
        std::string mFilePath;
        int mTriangleCount = 0;
        int mVertexCount = 0;
        int mIndexCount = 0;
    
        // Model Data
        tinygltf::Model mtinyModel;
        std::shared_ptr<XEngine::graphics::GLTFStaticMesh> mModel;
    
        // SSBO Handles (OpenGL IDs)
        GLuint mOBVHSSBO = 0;           // Binding 2
        GLuint mVerticesSSBO = 0;       // Binding 3
        GLuint mIndicesSSBO = 0;        // Binding 4
        GLuint mFaceNormalsSSBO = 0;    // Binding 5
        GLuint mTexCoordsSSBO = 0;      // Binding 6
        GLuint mMaterialIndicesSSBO = 0;// Binding 7
        GLuint mMaterialDataSSBO = 0;   // Binding 8
        GLuint mNormalsSSBO = 0;        // Binding 9
        GLuint mPackedTriSSBO = 0;      // Binding 15
    
        // Texture
        GLuint EnvTextureSSBO = 0;      // Binding 12
};
