#include "graphics/Gbuffer.h"
#include "log.h"
#include "glad/glad.h"

namespace XEngine::graphics 
{

    GBuffer::GBuffer() : mFbo(0), mDepthTexture(0), mWidth(0), mHeight(0) {
        for (int i = 0; i < GBUFFER_NUM_TEXTURES; i++) {
            mTextures[i] = 0;
        }
    }

    GBuffer::~GBuffer() {
        cleanUp();
    }

    void GBuffer::cleanUp() {
        if (mFbo != 0) {
            glDeleteFramebuffers(1, &mFbo);
        }
        if (mTextures[0] != 0) {
            glDeleteTextures(GBUFFER_NUM_TEXTURES, mTextures);
        }
        if (mDepthTexture != 0) {
            glDeleteTextures(1, &mDepthTexture);
        }
    }

    bool GBuffer::initialize(unsigned int width, unsigned int height) {
        cleanUp();

        mWidth = width;
        mHeight = height;

        glGenFramebuffers(1, &mFbo);
        glBindFramebuffer(GL_FRAMEBUFFER, mFbo);

        glGenTextures(GBUFFER_NUM_TEXTURES, mTextures);
        glGenTextures(1, &mDepthTexture);

        // 1. Position: 使用 32F 以確保光追時的世界座標重建精確
        glBindTexture(GL_TEXTURE_2D, mTextures[GBUFFER_POSITION]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mTextures[GBUFFER_POSITION], 0);

        // 2. Normal: 16F 足夠
        glBindTexture(GL_TEXTURE_2D, mTextures[GBUFFER_NORMAL]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, mTextures[GBUFFER_NORMAL], 0);

        // 3. Albedo + Metallic: RGB=Albedo, A=Metallic
        glBindTexture(GL_TEXTURE_2D, mTextures[GBUFFER_ALBEDO]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, mTextures[GBUFFER_ALBEDO], 0);

        // 4. Emission + Roughness: RGB=Emission, A=Roughness
        glBindTexture(GL_TEXTURE_2D, mTextures[GBUFFER_EMISSION]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, mTextures[GBUFFER_EMISSION], 0);

        // Depth Attachment
        glBindTexture(GL_TEXTURE_2D, mDepthTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, mDepthTexture, 0);

        unsigned int attachments[4] = {
            GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3
        };
        glDrawBuffers(4, attachments);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            XENGINE_ERROR("GBuffer Framebuffer not complete!");
            return false;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return true;
    }

    void GBuffer::bindForWriting() {
        glBindFramebuffer(GL_FRAMEBUFFER, mFbo);
        glViewport(0, 0, mWidth, mHeight);
        glEnable(GL_DEPTH_TEST);
        // 背景色設為 0 (alpha=0)，Compute Shader 讀到 0 會視為背景/天空
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void GBuffer::bindForReading(unsigned int startSlot) {
        for (unsigned int i = 0; i < GBUFFER_NUM_TEXTURES; i++) {
            glActiveTexture(GL_TEXTURE0 + startSlot + i);
            glBindTexture(GL_TEXTURE_2D, mTextures[i]);
        }

        // glActiveTexture(GL_TEXTURE0 + startSlot + GBUFFER_NUM_TEXTURES);
        // glBindTexture(GL_TEXTURE_2D, mDepthTexture);
    }

    GLuint GBuffer::getTexture(GBUFFER_TEXTURE_TYPE type) {
        if (type >= 0 && type < GBUFFER_NUM_TEXTURES) {
            return mTextures[type];
        }
        return 0;
    }

    void GBuffer::resize(unsigned int width, unsigned int height) {
        if (mWidth != width || mHeight != height) {
            initialize(width, height);
        }
    }

    void GBuffer::unbind()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

}