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

        for (unsigned int i = 0; i < GBUFFER_NUM_TEXTURES; i++) {
            glBindTexture(GL_TEXTURE_2D, mTextures[i]);


            GLenum internalFormat = GL_RGBA16F;
            GLenum format = GL_RGBA;
            GLenum type = GL_FLOAT;

            glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, type, NULL);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, mTextures[i], 0);
        }

        // Depth Attachment
        glBindTexture(GL_TEXTURE_2D, mDepthTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, mDepthTexture, 0);

        unsigned int attachments[GBUFFER_NUM_TEXTURES] = {
            GL_COLOR_ATTACHMENT0,
            GL_COLOR_ATTACHMENT1,
            GL_COLOR_ATTACHMENT2,
            GL_COLOR_ATTACHMENT3
        };
        glDrawBuffers(GBUFFER_NUM_TEXTURES, attachments);

        GLenum Status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (Status != GL_FRAMEBUFFER_COMPLETE) {
            XENGINE_ERROR("GBuffer FBO Error, Status: 0x {}",Status);
            return false;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return true;
    }

    void GBuffer::bindForWriting() {
        glBindFramebuffer(GL_FRAMEBUFFER, mFbo);
        glViewport(0, 0, mWidth, mHeight);
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
}