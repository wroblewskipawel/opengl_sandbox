#pragma once

#include <glad/glad.h>

#include <stdexcept>
#include <string>

using namespace std::string_literals;

namespace gl {

class Framebuffer {
   public:
    enum class BindPoint : GLenum {
        Draw = GL_DRAW_FRAMEBUFFER,
        Read = GL_READ_FRAMEBUFFER,
    };

    enum class Attachment : GLenum {
        Color_0 = GL_COLOR_ATTACHMENT0,
        Color_1 = GL_COLOR_ATTACHMENT1,
        Color_2 = GL_COLOR_ATTACHMENT2,
        Color_3 = GL_COLOR_ATTACHMENT3,
        Color_4 = GL_COLOR_ATTACHMENT4,
        Color_5 = GL_COLOR_ATTACHMENT5,
        Color_6 = GL_COLOR_ATTACHMENT6,
        Color_7 = GL_COLOR_ATTACHMENT7,
        Depth = GL_DEPTH_ATTACHMENT,
        Stencil = GL_STENCIL_ATTACHMENT,
        DepthStencil = GL_DEPTH_STENCIL_ATTACHMENT,
    };

    enum class Buffer : GLenum {
        Color_0 = GL_COLOR_ATTACHMENT0,
        Color_1 = GL_COLOR_ATTACHMENT1,
        Color_2 = GL_COLOR_ATTACHMENT2,
        Color_3 = GL_COLOR_ATTACHMENT3,
        Color_4 = GL_COLOR_ATTACHMENT4,
        Color_5 = GL_COLOR_ATTACHMENT5,
        Color_6 = GL_COLOR_ATTACHMENT6,
        Color_7 = GL_COLOR_ATTACHMENT7,
        None = GL_NONE,
    };

    Framebuffer() { glCreateFramebuffers(1, &m_glFramebuffer); }

    Framebuffer(const Framebuffer&) = delete;
    Framebuffer(Framebuffer&& other) : m_glFramebuffer{other.m_glFramebuffer} {
        other.m_glFramebuffer = GL_NONE;
    };

    Framebuffer& operator=(const Framebuffer&) = delete;
    Framebuffer& operator=(Framebuffer&& other) {
        m_glFramebuffer = other.m_glFramebuffer;
        other.m_glFramebuffer = GL_NONE;
        return *this;
    };

    ~Framebuffer() {
        if (m_glFramebuffer != GL_NONE) {
            glDeleteFramebuffers(1, &m_glFramebuffer);
        }
    }

    void bind(BindPoint binding) {
        if (currentFramebuffer != m_glFramebuffer) {
            glBindFramebuffer(static_cast<GLenum>(binding), m_glFramebuffer);
            currentFramebuffer = m_glFramebuffer;
        }
    }

    void setAttachment(Attachment attachment, GLuint texture, GLint level = 0) {
        glNamedFramebufferTexture(
            m_glFramebuffer, static_cast<GLenum>(attachment), texture, level);
    }

    void setDrawBuffers(const std::vector<Buffer>& buffers) {
        glNamedFramebufferDrawBuffers(
            m_glFramebuffer, buffers.size(),
            reinterpret_cast<const GLenum*>(buffers.data()));
    }

    void setReadBuffer(Buffer buffer) {
        glNamedFramebufferReadBuffer(m_glFramebuffer,
                                     static_cast<GLenum>(buffer));
    }

    static void bindDefault(BindPoint binding) {
        if (currentFramebuffer != 0) {
            currentFramebuffer = 0;
            glBindFramebuffer(static_cast<GLenum>(binding), 0);
        }
    }

   private:
    inline static GLuint currentFramebuffer{0};

    GLuint m_glFramebuffer;
};

}  // namespace gl
