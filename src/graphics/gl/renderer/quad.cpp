#include "rupture/graphics/gl/renderer/quad.h"

#include <vector>

#include "rupture/graphics/gl/renderer/uniform.h"
#include "rupture/graphics/gl/texture.h"
#include "rupture/graphics/gltf/texture.h"
#include "rupture/graphics/shaders/shaders.h"

namespace gl {

QuadRenderer::QuadRenderer()
    : m_brdfMapShader{shader::core::BRDF_MAP},
      m_texturedQuadShader{shader::core::TEXTURED_QUAD} {
    createVertexBuffer();
    createVertexArray();
}

void QuadRenderer::createVertexBuffer() {
    std::vector<QuadVert> quadMesh{
        {glm::vec2{-1.0f, -1.0f}, glm::vec2{0.0f, 0.0f}},
        {glm::vec2{1.0f, -1.0f}, glm::vec2{1.0f, 0.0f}},
        {glm::vec2{1.0f, 1.0f}, glm::vec2{1.0f, 1.0f}},
        {glm::vec2{-1.0f, 1.0f}, glm::vec2{0.0f, 1.0f}}};
    std::vector<uint32_t> quadIndices{0, 1, 2, 2, 3, 0};

    glCreateBuffers(1, &m_vertexBuffer);
    glCreateBuffers(1, &m_indexBuffer);

    glNamedBufferStorage(m_vertexBuffer, sizeof(QuadVert) * quadMesh.size(),
                         quadMesh.data(), GL_NONE);
    glNamedBufferStorage(m_indexBuffer, sizeof(uint32_t) * quadIndices.size(),
                         quadIndices.data(), GL_NONE);
}

void QuadRenderer::createVertexArray() {
    glCreateVertexArrays(1, &m_vertexArray);

    glVertexArrayVertexBuffer(m_vertexArray, 0, m_vertexBuffer, 0,
                              sizeof(QuadVert));
    glVertexArrayElementBuffer(m_vertexArray, m_indexBuffer);

    glVertexArrayAttribBinding(m_vertexArray, 0, 0);
    glVertexArrayAttribFormat(m_vertexArray, 0, 2, GL_FLOAT, GL_FALSE,
                              offsetof(QuadVert, pos));
    glEnableVertexArrayAttrib(m_vertexArray, 0);

    glVertexArrayAttribBinding(m_vertexArray, 1, 0);
    glVertexArrayAttribFormat(m_vertexArray, 1, 2, GL_FLOAT, GL_TRUE,
                              offsetof(QuadVert, tex));
    glEnableVertexArrayAttrib(m_vertexArray, 1);
}

void QuadRenderer::bind() {
    if (m_vertexArray != currentVertexArray) {
        currentVertexArray = m_vertexArray;
        glBindVertexArray(m_vertexArray);
    }
}

Texture QuadRenderer::createBRDFMap(size_t width, size_t height) {
    Texture::SamplerConfig sampler;

    sampler.magFilter = GL_LINEAR;
    sampler.minFilter = GL_LINEAR;
    sampler.wrapS = GL_CLAMP_TO_EDGE;
    sampler.wrapT = GL_CLAMP_TO_EDGE;

    Texture brdfMap{GL_TEXTURE_2D, GL_RG16F, width, height, sampler, 1};

    std::array<GLint, 4> currentViewport{};
    glGetIntegerv(GL_VIEWPORT, currentViewport.data());

    bind();

    m_brdfMapShader.use();

    m_framebuffer.bind(Framebuffer::BindPoint::Draw);

    m_framebuffer.setAttachment(Framebuffer::Attachment::Color_0,
                                brdfMap.texture());
    m_framebuffer.setDrawBuffers({Framebuffer::Buffer::Color_0});

    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    Framebuffer::bindDefault(Framebuffer::BindPoint::Draw);

    m_framebuffer.setAttachment(Framebuffer::Attachment::Color_0, GL_NONE);
    m_framebuffer.setDrawBuffers({Framebuffer::Buffer::None});

    glViewport(0, 0, currentViewport[2], currentViewport[3]);

    return brdfMap;
}

void QuadRenderer::drawTexture(Texture& texture) {
    bind();
    m_texturedQuadShader.use();

    glBindTextureUnit(0, texture.texture());
    m_texturedQuadShader.setUniformI(renderer::uniforms::quad::TEXTURE, 0);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

}  // namespace gl
