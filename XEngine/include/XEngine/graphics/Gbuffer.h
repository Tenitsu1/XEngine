#pragma once
#pragma once

#include <vector>
#include <iostream>

namespace XEngine::graphics
{
    class GBuffer {
    public:
        enum GBUFFER_TEXTURE_TYPE {
            GBUFFER_POSITION = 0,   
            GBUFFER_NORMAL,         
            GBUFFER_ALBEDO,         
            GBUFFER_EMISSION,       
            GBUFFER_NUM_TEXTURES
        };

        GBuffer();
        ~GBuffer();

        bool initialize(unsigned int width, unsigned int height);

        void bindForWriting();

        void bindForReading(unsigned int startSlot = 0);

        uint32_t getTexture(GBUFFER_TEXTURE_TYPE type);

        uint32_t getDepthTexture() { return mDepthTexture; }
        uint32_t getFBO() { return mFbo; }

        void resize(unsigned int width, unsigned int height);

    private:
        uint32_t mFbo;
        uint32_t mTextures[GBUFFER_NUM_TEXTURES];
        uint32_t mDepthTexture;
        unsigned int mWidth;
        unsigned int mHeight;

        void cleanUp();
    };

}