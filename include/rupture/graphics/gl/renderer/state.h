#pragma once

#include <glad/glad.h>

namespace gl {

class RendererState {
   public:
    virtual ~RendererState() = default;

   protected:
    static const GLuint vertexBufferIndex{0};
    static const GLuint instanceBufferIndex{1};

    inline static GLuint currentVertexArray{GL_NONE};
};

}  // namespace gl
