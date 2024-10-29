#pragma once

#include <glad/glad.h>

#include <string>
#include <vector>

#include "rupture/graphics/gl/block/std140.h"
#include "rupture/graphics/gl/shader.h"

namespace gl {

class ShaderUniform : public UniformBlock {
   public:
    ShaderUniform(const std::string& name) : m_name{name} {
        glCreateBuffers(1, &m_glBuffer);
    }

    ShaderUniform(const ShaderUniform&) = delete;
    ShaderUniform(ShaderUniform&& other)
        : m_name{std::move(other.m_name)}, m_glBuffer{other.m_glBuffer} {
        other.m_glBuffer = GL_NONE;
    }

    ShaderUniform& operator=(const ShaderUniform&) = delete;
    ShaderUniform& operator=(ShaderUniform&& other) {
        if (m_glBuffer != GL_NONE) {
            glDeleteBuffers(1, &m_glBuffer);
        }
        m_glBuffer = other.m_glBuffer;
        m_name = std::move(other.m_name);
        other.m_glBuffer = GL_NONE;
        other.m_name = ""s;
        return *this;
    }

    ~ShaderUniform() {
        if (m_glBuffer != GL_NONE) {
            glDeleteBuffers(1, &m_glBuffer);
        }
    }

    template <typename... Types>
    void write(const std140::AlignedArray<Types...>& data,
               GLenum storageFlags = GL_STREAM_DRAW) {
        glNamedBufferData(m_glBuffer, sizeof(std140::AlignedArray<Types...>),
                          &data, storageFlags);
    }

    template <typename... Types>
    void write(const std140::Block<Types...>& data,
               GLenum storageFlags = GL_STREAM_DRAW) {
        glNamedBufferData(m_glBuffer, sizeof(std140::Block<Types...>), &data,
                          storageFlags);
    }

    template <typename Type>
    void write(const std::vector<Type>& data,
               GLenum storageFlags = GL_STREAM_DRAW) {
        glNamedBufferData(m_glBuffer, data.size() * sizeof(Type), data.data(),
                          storageFlags);
    }

   private:
    const std::string& blockName() const override { return m_name; };
    GLuint glBuffer() const override { return m_glBuffer; };

    std::string m_name;
    GLuint m_glBuffer;
};

}  // namespace gl