#pragma once

#include <glad/glad.h>

#include <glm/glm.hpp>

#include "rupture/graphics/gl/framebuffer.h"
#include "rupture/graphics/gl/renderer/state.h"
#include "rupture/graphics/gl/shader.h"
#include "rupture/graphics/gl/texture.h"

namespace gl {

class QuadRenderer : public RendererState {
   public:
    QuadRenderer();

    QuadRenderer(const QuadRenderer&) = delete;
    QuadRenderer(QuadRenderer&&) = delete;

    QuadRenderer& operator=(const QuadRenderer&) = delete;
    QuadRenderer& operator=(QuadRenderer&&) = delete;

    ~QuadRenderer() override {
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

    void drawTexture(Texture& texture);

    Texture createBRDFMap(size_t width, size_t height);

   private:
    struct QuadVert {
        glm::vec2 pos;
        glm::vec2 tex;
    };

    void bind();

    void createVertexArray();
    void createVertexBuffer();

    Shader m_brdfMapShader;
    Shader m_texturedQuadShader;

    Framebuffer m_framebuffer;

    GLuint m_vertexArray;
    GLuint m_vertexBuffer;
    GLuint m_indexBuffer;
};

}  // namespace gl