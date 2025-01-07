#include "rupture/graphics/gl/renderer/cube.h"
#include "rupture/graphics/gl/renderer/uniform.h"

#include <glm/glm.hpp>

#include "rupture/graphics/gl/texture.h"
#include "rupture/graphics/gltf/texture.h"
#include "rupture/graphics/shaders/shaders.h"

namespace gl {
CubeRenderer::CubeRenderer()
    : m_projections{renderer::uniforms::cube::PROJECTION},
      m_depthCubeShader{shader::core::DEPTH_CUBE},
      m_colorCubeShader{shader::core::COLOR_CUBE},
      m_irradianceMapShader{shader::core::IRRADIANCE_MAP},
      m_specularMapShader{shader::core::SPECULAR_MAP},
      m_skyboxShader{shader::core::SKYBOX} {
    createVertexBuffer();
    createVertexArray();
    writeProjectionUniform();
}

void CubeRenderer::createVertexArray() {
    glCreateVertexArrays(1, &m_vertexArray);

    glVertexArrayVertexBuffer(m_vertexArray, 0, m_vertexBuffer, 0,
                              sizeof(glm::vec3));
    glVertexArrayElementBuffer(m_vertexArray, m_indexBuffer);

    glVertexArrayAttribBinding(m_vertexArray, 0, 0);
    glVertexArrayAttribFormat(m_vertexArray, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glEnableVertexArrayAttrib(m_vertexArray, 0);
};

void CubeRenderer::createVertexBuffer() {
    std::vector<glm::vec3> cubeMesh{
        glm::vec3{-1.0f, -1.0f, -1.0f}, glm::vec3{1.0f, -1.0f, -1.0f},
        glm::vec3{1.0f, 1.0f, -1.0f},   glm::vec3{-1.0f, 1.0f, -1.0f},
        glm::vec3{-1.0f, -1.0f, 1.0f},  glm::vec3{1.0f, -1.0f, 1.0f},
        glm::vec3{1.0f, 1.0f, 1.0f},    glm::vec3{-1.0f, 1.0f, 1.0f},
    };
    std::vector<uint32_t> cubeIndices{0, 2, 1, 0, 3, 2, 0, 1, 5, 5, 4, 0,
                                      1, 2, 5, 5, 2, 6, 4, 5, 6, 6, 7, 4,
                                      0, 4, 7, 7, 3, 0, 3, 6, 2, 7, 6, 3};

    glCreateBuffers(1, &m_vertexBuffer);
    glCreateBuffers(1, &m_indexBuffer);

    glNamedBufferStorage(m_vertexBuffer, sizeof(glm::vec3) * cubeMesh.size(),
                         cubeMesh.data(), GL_NONE);
    glNamedBufferStorage(m_indexBuffer, sizeof(uint32_t) * cubeIndices.size(),
                         cubeIndices.data(), GL_NONE);
}

void CubeRenderer::writeProjectionUniform() {
    auto proj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);

    auto views = std::vector{
        proj * glm::lookAt(glm::vec3{0.0f}, glm::vec3{1.0f, 0.0f, 0.0f},
                           glm::vec3{0.0f, -1.0f, 0.0f}),
        proj * glm::lookAt(glm::vec3{0.0f}, glm::vec3{-1.0f, 0.0f, 0.0f},
                           glm::vec3{0.0f, -1.0f, 0.0f}),
        proj * glm::lookAt(glm::vec3{0.0f}, glm::vec3{0.0f, 1.0f, 0.0f},
                           glm::vec3{0.0f, 0.0f, 1.0f}),
        proj * glm::lookAt(glm::vec3{0.0f}, glm::vec3{0.0f, -1.0f, 0.0f},
                           glm::vec3{0.0f, 0.0f, -1.0f}),
        proj * glm::lookAt(glm::vec3{0.0f}, glm::vec3{0.0f, 0.0f, 1.0f},
                           glm::vec3{0.0f, -1.0f, 0.0f}),
        proj * glm::lookAt(glm::vec3{0.0f}, glm::vec3{0.0f, 0.0f, -1.0f},
                           glm::vec3{0.0f, -1.0f, 0.0f})};

    m_projections.write(views, GL_STATIC_DRAW);
}

void CubeRenderer::bind() {
    if (m_vertexArray != currentVertexArray) {
        currentVertexArray = m_vertexArray;
        glBindVertexArray(m_vertexArray);
    }
}

void CubeRenderer::drawSkybox(Environment& environment,
                              const glm::mat4& cameraMatrix,
                              const glm::vec3& cameraPosition) {
    Framebuffer::bindDefault(Framebuffer::BindPoint::Draw);

    glCullFace(GL_FRONT);
    glDisable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT);

    bind();

    auto model = glm::translate(glm::mat4{1.0f}, cameraPosition);

    m_skyboxShader.use();
    m_skyboxShader.setUniformMatrix4(Shader::CAMERA_MATRIX_UNIFORM,
                                     cameraMatrix);
    m_skyboxShader.setUniformMatrix4(renderer::uniforms::cube::MODEL, model);

    auto environmentTexture = environment.environmentMap().texture();
    glBindTextureUnit(0, environmentTexture);
    m_skyboxShader.setUniformI(renderer::uniforms::cube::ENV_CUBE_TEXTURE, 0);

    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

    glEnable(GL_DEPTH_TEST);
    glCullFace(GL_BACK);
}

Environment CubeRenderer::createEnvironmentMap(
    const std::filesystem::path& path, size_t cubeResolution,
    size_t mipLevels) {
    gltf::HDRTexture texture(path);
    Texture environment2D{texture};

    std::array<GLint, 4> currentViewport{};
    glGetIntegerv(GL_VIEWPORT, currentViewport.data());

    glViewport(0, 0, cubeResolution, cubeResolution);
    glCullFace(GL_FRONT);
    glDisable(GL_DEPTH_TEST);

    bind();

    m_framebuffer.bind(Framebuffer::BindPoint::Draw);
    m_framebuffer.setDrawBuffers({Framebuffer::Buffer::Color_0});

    auto environmentCube = renderEnvironmentMap(environment2D, cubeResolution);
    auto irradianceCube = renderIrradianceMap(environmentCube, cubeResolution);
    auto specularCube =
        renderSpecularMap(environmentCube, cubeResolution, mipLevels);

    m_framebuffer.setAttachment(Framebuffer::Attachment::Color_0, GL_NONE);
    m_framebuffer.setDrawBuffers({Framebuffer::Buffer::None});

    Framebuffer::bindDefault(Framebuffer::BindPoint::Draw);
    glEnable(GL_DEPTH_TEST);
    glCullFace(GL_BACK);

    glViewport(0, 0, currentViewport[2], currentViewport[3]);

    return Environment{std::move(environmentCube), std::move(irradianceCube),
                       std::move(specularCube)};
}

Texture CubeRenderer::renderEnvironmentMap(Texture& environment2D,
                                           size_t cubeResolution) {
    Texture::SamplerConfig sampler;

    sampler.magFilter = GL_LINEAR;
    sampler.minFilter = GL_LINEAR;
    sampler.wrapS = GL_CLAMP_TO_EDGE;
    sampler.wrapT = GL_CLAMP_TO_EDGE;

    Texture environmentCube{GL_TEXTURE_CUBE_MAP, GL_RGB16F, cubeResolution,
                            cubeResolution,      sampler,   1};

    m_framebuffer.setAttachment(Framebuffer::Attachment::Color_0,
                                environmentCube.texture());

    m_colorCubeShader.use();

    glBindTextureUnit(0, environment2D.texture());
    m_colorCubeShader.setUniformI(renderer::uniforms::cube::ENV_2D_TEXTURE, 0);
    m_colorCubeShader.setUniformMatrix4(renderer::uniforms::cube::MODEL, glm::mat4{1.0f});
    m_colorCubeShader.bindUniformBlock(m_projections.blockInfo());

    glClear(GL_COLOR_BUFFER_BIT);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

    return environmentCube;
}

Texture CubeRenderer::renderIrradianceMap(Texture& environmentCube,
                                          size_t cubeResolution) {
    Texture::SamplerConfig sampler;

    sampler.magFilter = GL_LINEAR;
    sampler.minFilter = GL_LINEAR;
    sampler.wrapS = GL_CLAMP_TO_EDGE;
    sampler.wrapT = GL_CLAMP_TO_EDGE;

    Texture irradianceCube{GL_TEXTURE_CUBE_MAP, GL_RGB16F, cubeResolution,
                           cubeResolution,      sampler,   1};

    m_framebuffer.setAttachment(Framebuffer::Attachment::Color_0,
                                irradianceCube.texture());

    m_irradianceMapShader.use();

    glBindTextureUnit(0, environmentCube.texture());
    m_irradianceMapShader.setUniformI(renderer::uniforms::cube::ENV_CUBE_TEXTURE, 0);
    m_irradianceMapShader.setUniformMatrix4(renderer::uniforms::cube::MODEL, glm::mat4{1.0f});
    m_irradianceMapShader.bindUniformBlock(m_projections.blockInfo());

    glClear(GL_COLOR_BUFFER_BIT);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

    return irradianceCube;
}

Texture CubeRenderer::renderSpecularMap(Texture& environmentCube,
                                        size_t cubeResolution,
                                        size_t mipLevels) {
    Texture::SamplerConfig sampler;

    sampler.magFilter = GL_LINEAR;
    sampler.minFilter = GL_LINEAR_MIPMAP_LINEAR;
    sampler.wrapS = GL_CLAMP_TO_EDGE;
    sampler.wrapT = GL_CLAMP_TO_EDGE;

    Texture specularCube{GL_TEXTURE_CUBE_MAP, GL_RGB16F, cubeResolution,
                         cubeResolution,      sampler,   mipLevels};

    m_specularMapShader.use();

    glBindTextureUnit(0, environmentCube.texture());
    m_specularMapShader.setUniformI(renderer::uniforms::cube::ENV_CUBE_TEXTURE, 0);
    m_specularMapShader.setUniformMatrix4(renderer::uniforms::cube::MODEL, glm::mat4{1.0f});
    m_specularMapShader.bindUniformBlock(m_projections.blockInfo());

    for (size_t l{0}; l < mipLevels; l++) {
        m_framebuffer.setAttachment(Framebuffer::Attachment::Color_0,
                                    specularCube.texture(), l);

        size_t mipResolution = cubeResolution * glm::pow(0.5, l);
        float roughness =
            static_cast<float>(l) / static_cast<float>(mipLevels - 1);

        m_specularMapShader.setUniformF(renderer::uniforms::cube::ROUGHNESS, roughness);

        glViewport(0, 0, mipResolution, mipResolution);
        glClear(GL_COLOR_BUFFER_BIT);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    }

    return specularCube;
}

void CubeRenderer::createOmniShadowMap() {}

}  // namespace gl