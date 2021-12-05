#include "graphics/gl/texture.h"

#include <iostream>

namespace gl {

Texture::Texture(GLenum type, GLenum sizedFormat, size_t width, size_t height,
                 SamplerConfig sampler, size_t mipLevels) {
    glCreateTextures(type, 1, &m_glTexture);
    glTextureStorage2D(m_glTexture, mipLevels, sizedFormat, width, height);

    glTextureParameteri(m_glTexture, GL_TEXTURE_MIN_FILTER, sampler.minFilter);
    glTextureParameteri(m_glTexture, GL_TEXTURE_MAG_FILTER, sampler.magFilter);
    glTextureParameteri(m_glTexture, GL_TEXTURE_WRAP_S, sampler.wrapS);
    glTextureParameteri(m_glTexture, GL_TEXTURE_WRAP_T, sampler.wrapT);

    m_format = sizedFormat;
    m_bindlessHandle = glGetTextureHandleARB(m_glTexture);
    textureHandles.insert(m_bindlessHandle);
}

Texture::Texture(const gltf::Texture& source, size_t mipLevels) {
    auto [format, sized_format] = getGLFormat(source.format());
    glCreateTextures(GL_TEXTURE_2D, 1, &m_glTexture);
    glTextureStorage2D(m_glTexture, mipLevels, sized_format, source.width(),
                       source.height());
    glTextureSubImage2D(m_glTexture, 0, 0, 0, source.width(), source.height(),
                        format, GL_UNSIGNED_BYTE, source.data().data());
    glGenerateTextureMipmap(m_glTexture);

    SamplerConfig sampler{};

    glTextureParameteri(m_glTexture, GL_TEXTURE_MIN_FILTER, sampler.minFilter);
    glTextureParameteri(m_glTexture, GL_TEXTURE_MAG_FILTER, sampler.magFilter);
    glTextureParameteri(m_glTexture, GL_TEXTURE_WRAP_S, sampler.wrapS);
    glTextureParameteri(m_glTexture, GL_TEXTURE_WRAP_T, sampler.wrapT);

    m_format = sized_format;
    m_bindlessHandle = glGetTextureHandleARB(m_glTexture);
    textureHandles.insert(m_bindlessHandle);
}

Texture::Texture(const gltf::HDRTexture& source, size_t mipLevels) {
    glCreateTextures(GL_TEXTURE_2D, 1, &m_glTexture);
    glTextureStorage2D(m_glTexture, mipLevels, GL_RGB16F, source.width(),
                       source.height());
    glTextureSubImage2D(m_glTexture, 0, 0, 0, source.width(), source.height(),
                        GL_RGB, GL_FLOAT, source.data().data());

    SamplerConfig sampler{};

    glTextureParameteri(m_glTexture, GL_TEXTURE_MIN_FILTER, sampler.minFilter);
    glTextureParameteri(m_glTexture, GL_TEXTURE_MAG_FILTER, sampler.magFilter);
    glTextureParameteri(m_glTexture, GL_TEXTURE_WRAP_S, sampler.wrapS);
    glTextureParameteri(m_glTexture, GL_TEXTURE_WRAP_T, sampler.wrapT);

    m_format = GL_RGB16F;
    m_bindlessHandle = glGetTextureHandleARB(m_glTexture);
    textureHandles.insert(m_bindlessHandle);
}

}  // namespace gl