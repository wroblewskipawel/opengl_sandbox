#pragma once

#include <glad/glad.h>

#include <cstring>
#include <iostream>
#include <unordered_set>

#include "graphics/gltf/texture.h"

namespace gl {

class Texture;

class Texture {
   public:
    static const size_t DEFAULT_MIP_LEVELS = 8;

    struct SamplerConfig {
        GLenum minFilter = GL_LINEAR_MIPMAP_LINEAR;
        GLenum magFilter = GL_LINEAR;
        GLenum wrapS = GL_REPEAT;
        GLenum wrapT = GL_REPEAT;
    };

    enum class Type : GLenum {
        Texture2D = GL_TEXTURE_2D,
        CubeMap = GL_TEXTURE_CUBE_MAP
    };

    Texture() { m_glTexture = GL_NONE; }
    Texture(GLenum type, GLenum sizedFormat, size_t width, size_t height,
            SamplerConfig sampler, size_t mipLevels = DEFAULT_MIP_LEVELS);
    Texture(const gltf::Texture& source, size_t mipLevels = DEFAULT_MIP_LEVELS);
    Texture(const gltf::HDRTexture& source,
            size_t mipLevels = DEFAULT_MIP_LEVELS);

    ~Texture() {
        if (m_glTexture) {
            if (residentHandles.count(m_bindlessHandle)) {
                glMakeTextureHandleNonResidentARB(m_bindlessHandle);
                residentHandles.erase(m_bindlessHandle);
            }
            textureHandles.erase(m_bindlessHandle);
            glDeleteTextures(1, &m_glTexture);
        }
    }

    Texture& operator=(const Texture&) = delete;
    Texture& operator=(Texture&& other) {
        m_glTexture = other.m_glTexture;
        m_bindlessHandle = other.m_bindlessHandle;
        m_format = other.m_format;
        other.m_glTexture = GL_NONE;
        other.m_bindlessHandle = GL_NONE;
        return *this;
    };

    Texture(const Texture&) = delete;
    Texture(Texture&& other)
        : m_bindlessHandle{other.m_bindlessHandle},
          m_glTexture{other.m_glTexture},
          m_format{other.m_format} {
        other.m_glTexture = GL_NONE;
        other.m_bindlessHandle = GL_NONE;
    }

    GLuint64 handle() const { return m_bindlessHandle; }
    GLuint texture() { return m_glTexture; }

    static void makeResident(GLuint64 handle) {
        if (textureHandles.count(handle) && !residentHandles.count(handle)) {
            glMakeTextureHandleResidentARB(handle);
            residentHandles.insert(handle);
        }
    }
    static void makeNonResident(GLuint64 handle) {
        if (textureHandles.count(handle) && residentHandles.count(handle)) {
            glMakeTextureHandleNonResidentARB(handle);
            residentHandles.erase(handle);
        }
    }

    GLenum format() const { return m_format; }

   private:
    Texture(GLuint texture, GLenum format)
        : m_glTexture{texture}, m_format{format} {}

    inline static std::unordered_set<GLuint64> residentHandles;
    inline static std::unordered_set<GLuint64> textureHandles;

    static constexpr std::pair<GLenum, GLenum> getGLFormat(
        gltf::Texture::Format format) {
        switch (format) {
            case gltf::Texture::Format::Grey:
                return {GL_RED, GL_R8};
            case gltf::Texture::Format::GreyAlpha:
                return {GL_RG, GL_RG8};
            case gltf::Texture::Format::RGB:
                return {GL_RGB, GL_RGB8};
            case gltf::Texture::Format::RGBA:
                return {GL_RGBA, GL_RGBA8};
            default:
                return {GL_FALSE, GL_FALSE};
        }
    }

    GLuint64 m_bindlessHandle{GL_NONE};
    GLuint m_glTexture{GL_NONE};
    GLenum m_format{GL_NONE};
    GLenum m_type{GL_NONE};
};

}  // namespace gl