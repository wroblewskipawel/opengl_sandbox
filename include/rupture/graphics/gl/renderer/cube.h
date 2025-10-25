#pragma once

#include <glad/glad.h>

#include "rupture/graphics/gl/environment.h"
#include "rupture/graphics/gl/framebuffer.h"
#include "rupture/graphics/gl/renderer/state.h"
#include "rupture/graphics/gl/shader.h"
#include "rupture/graphics/gl/texture.h"
#include "rupture/graphics/gl/uniform.h"

namespace gl {

class CubeRenderer : public RendererState {
   public:
    CubeRenderer();

    CubeRenderer(const CubeRenderer&) = delete;
    CubeRenderer(CubeRenderer&&) = delete;

    CubeRenderer& operator=(const CubeRenderer&) = delete;
    CubeRenderer& operator=(CubeRenderer&&) = delete;

    ~CubeRenderer() override {
        if (m_vertexArray != GL_NONE) {
            if (currentVertexArray == m_vertexArray) {
                currentVertexArray = GL_NONE;
                glBindVertexArray(GL_NONE);
            }
            glDeleteVertexArrays(1, &m_vertexArray);
            glDeleteBuffers(1, &m_vertexBuffer);
            glDeleteBuffers(1, &m_indexBuffer);
        }
    }

    void drawSkybox(Environment& environment, const glm::mat4& cameraMatrix,
                    const glm::vec3& cameraPositions);

    Environment createEnvironmentMap(const std::filesystem::path& path,
                                     size_t cubeResolution = 512,
                                     size_t mipLevels = 5);
    void createOmniShadowMap();

   private:
    void bind();

    void createVertexArray();
    void createVertexBuffer();
    void writeProjectionUniform();

    Texture renderEnvironmentMap(Texture& environment2D, size_t cubeResolution);
    Texture renderIrradianceMap(Texture& environmentCube,
                                size_t cubeResolution);
    Texture renderSpecularMap(Texture& environmentCube, size_t cubeResolution,
                              size_t mipLevels);

    ShaderUniform m_projections;

    Shader m_depthCubeShader;
    Shader m_colorCubeShader;
    Shader m_irradianceMapShader;
    Shader m_specularMapShader;
    Shader m_skyboxShader;

    Framebuffer m_framebuffer;

    GLuint m_vertexArray;
    GLuint m_vertexBuffer;
    GLuint m_indexBuffer;
};

}  // namespace gl