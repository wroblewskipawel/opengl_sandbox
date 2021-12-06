#include "graphics/gl/renderer.h"

#include <glm/glm.hpp>

#include "graphics/gl/texture.h"
#include "graphics/gltf/texture.h"

const std::string DEPTH_CUBE_SHADER_PATH{"shaders/utils/depth_cube"s};
const std::string COLOR_CUBE_SHADER_PATH{"shaders/utils/color_cube"s};
const std::string IRRADIANCE_MAP_SHADER_PATH{"shaders/utils/irradiance_map"s};
const std::string SPECULAR_MAP_SHADER_PATH{"shaders/utils/specular_map"s};
const std::string BRDF_MAP_SHADER_PATH{"shaders/utils/brdf_map"s};
const std::string SKYBOX_SHADER_PATH{"shaders/utils/skybox"s};
const std::string TEXTURED_QUAD_SHADER_PATH{"shaders/utils/textured_quad"s};

const std::string PROJECTION_UNIFORM{"CubeProjection"s};
const std::string ENV_2D_TEXTURE_UNIFORM{"equirectangular_map"s};
const std::string ENV_CUBE_TEXTURE_UNIFORM{"environment_map"s};
const std::string MODEL_UNIFORM{"model"s};
const std::string ROUGHNESS_UNIFORM{"roughness"s};
const std::string TEXTURE_UNIFORM("image"s);

namespace gl {

CubeRenderer::CubeRenderer()
    : m_projections{PROJECTION_UNIFORM},
      m_depthCubeShader{DEPTH_CUBE_SHADER_PATH},
      m_colorCubeShader{COLOR_CUBE_SHADER_PATH},
      m_irradianceMapShader{IRRADIANCE_MAP_SHADER_PATH},
      m_specularMapShader{SPECULAR_MAP_SHADER_PATH},
      m_skyboxShader{SKYBOX_SHADER_PATH} {
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
    m_skyboxShader.setUniformMatrix4(MODEL_UNIFORM, model);

    auto environmentTexture = environment.environmentMap().texture();
    glBindTextureUnit(0, environmentTexture);
    m_skyboxShader.setUniformI(ENV_CUBE_TEXTURE_UNIFORM, 0);

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
    m_colorCubeShader.setUniformI(ENV_2D_TEXTURE_UNIFORM, 0);
    m_colorCubeShader.setUniformMatrix4(MODEL_UNIFORM, glm::mat4{1.0f});
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
    m_irradianceMapShader.setUniformI(ENV_CUBE_TEXTURE_UNIFORM, 0);
    m_irradianceMapShader.setUniformMatrix4(MODEL_UNIFORM, glm::mat4{1.0f});
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
    m_specularMapShader.setUniformI(ENV_CUBE_TEXTURE_UNIFORM, 0);
    m_specularMapShader.setUniformMatrix4(MODEL_UNIFORM, glm::mat4{1.0f});
    m_specularMapShader.bindUniformBlock(m_projections.blockInfo());

    for (size_t l{0}; l < mipLevels; l++) {
        m_framebuffer.setAttachment(Framebuffer::Attachment::Color_0,
                                    specularCube.texture(), l);

        size_t mipResolution = cubeResolution * glm::pow(0.5, l);
        float roughness =
            static_cast<float>(l) / static_cast<float>(mipLevels - 1);

        m_specularMapShader.setUniformF(ROUGHNESS_UNIFORM, roughness);

        glViewport(0, 0, mipResolution, mipResolution);
        glClear(GL_COLOR_BUFFER_BIT);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    }

    return specularCube;
}

void CubeRenderer::createOmniShadowMap() {}

QuadRenderer::QuadRenderer()
    : m_brdfMapShader{BRDF_MAP_SHADER_PATH},
      m_texturedQuadShader{TEXTURED_QUAD_SHADER_PATH} {
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
    m_texturedQuadShader.setUniformI(TEXTURE_UNIFORM, 0);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

}  // namespace gl
