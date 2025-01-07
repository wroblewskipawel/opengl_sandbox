#pragma once

#include <glad/glad.h>

#include <condition_variable>
#include <future>
#include <iostream>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <type_traits>

#include "rupture/graphics/gl/attributes.h"
#include "rupture/graphics/gl/block/std140.h"
#include "rupture/graphics/gl/environment.h"
#include "rupture/graphics/gl/framebuffer.h"
#include "rupture/graphics/gl/model.h"
#include "rupture/graphics/gl/renderer/state.h"
#include "rupture/graphics/gl/shader.h"
#include "rupture/graphics/gl/uniform.h"
#include "rupture/graphics/gl/vertex_buffer.h"

namespace gl {

template <typename, typename, size_t, size_t>
class MeshRenderer;

template <size_t size>
class DrawIndicredBuffer {
   public:
    static constexpr size_t BufferSize = sizeof(command::Draw) * size;

    DrawIndicredBuffer(GLenum storageFlags = GL_DYNAMIC_DRAW) {
        glCreateBuffers(1, &m_glBuffer);
        glNamedBufferData(m_glBuffer, BufferSize, nullptr, storageFlags);
        m_commandCount = 0;
    };

    DrawIndicredBuffer(const DrawIndicredBuffer&) = delete;
    DrawIndicredBuffer(DrawIndicredBuffer&& other)
        : m_glBuffer{other.m_glBuffer} {
        other.m_glBuffer = GL_NONE;
    };

    DrawIndicredBuffer& operator=(const DrawIndicredBuffer&) = delete;
    DrawIndicredBuffer& operator=(DrawIndicredBuffer&& other) {
        if (m_glBuffer != GL_NONE) {
            glDeleteBuffers(1, &m_glBuffer);
        }
        m_glBuffer = other.m_glBuffer;
        other.m_glBuffer = GL_NONE;
        return *this;
    };

    ~DrawIndicredBuffer() {
        if (m_glBuffer != GL_NONE) {
            if (currentBuffer == m_glBuffer) {
                glBindBuffer(GL_DRAW_INDIRECT_BUFFER, GL_NONE);
                currentBuffer = GL_NONE;
            }
            glDeleteBuffers(1, &m_glBuffer);
        }
    }

    void write(const std::vector<command::Draw>& commands) {
        if (commands.size() > capacity) {
            throw std::logic_error("Insufficient draw indirect buffer size");
        }
        glNamedBufferSubData(m_glBuffer, 0,
                             commands.size() * sizeof(command::Draw),
                             commands.data());
        m_commandCount = commands.size();
    };

    void bind() {
        if (m_glBuffer != currentBuffer) {
            glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_glBuffer);
            currentBuffer = m_glBuffer;
        }
    }

    GLuint commandCount() const { return m_commandCount; };

    inline static const size_t capacity{size};

   private:
    inline static GLuint currentBuffer{GL_NONE};
    GLuint m_glBuffer;
    GLuint m_commandCount;
};

template <size_t size>
class MaterialIndexBuffer : public UniformBlock {
   public:
    static constexpr size_t BufferSize =
        sizeof(std140::AlignedArray<GLuint[size]>);

    MaterialIndexBuffer(GLenum storageFlags = GL_DYNAMIC_DRAW) {
        glCreateBuffers(1, &m_glBuffer);
        glNamedBufferData(m_glBuffer, BufferSize, nullptr, storageFlags);
    };

    MaterialIndexBuffer(const MaterialIndexBuffer&) = delete;
    MaterialIndexBuffer(MaterialIndexBuffer&& other)
        : m_glBuffer{other.m_glBuffer} {
        other.m_glBuffer = GL_NONE;
    };

    MaterialIndexBuffer& operator=(const MaterialIndexBuffer&) = delete;
    MaterialIndexBuffer& operator=(MaterialIndexBuffer&& other) {
        if (m_glBuffer != GL_NONE) {
            glDeleteBuffers(1, &m_glBuffer);
        }
        m_glBuffer = other.m_glBuffer;
        other.m_glBuffer = GL_NONE;
        return *this;
    };

    ~MaterialIndexBuffer() {
        if (m_glBuffer != GL_NONE) {
            glDeleteBuffers(1, &m_glBuffer);
        }
    }

    void write(const std::vector<GLuint>& indices) {
        if (indices.size() > capacity) {
            throw std::logic_error("Insufficient material index buffer size");
        }
        std140::AlignedArray<GLuint[size]> block{};
        for (size_t i{0}; i < indices.size(); i++) {
            block[i] = indices[i];
        }
        glNamedBufferSubData(m_glBuffer, 0, BufferSize, &block);
    }

   private:
    const std::string& blockName() const override { return name; };
    GLuint glBuffer() const override { return m_glBuffer; };

    inline static const std::string name{"MaterialIndices"s};
    static const size_t capacity{size};

    GLuint m_glBuffer;
};

template <typename Attrib, size_t Size>
class InstanceAttribBuffer {
   public:
    static constexpr size_t BufferSize = sizeof(Attrib) * Size;

    InstanceAttribBuffer(GLenum storageFlags = GL_DYNAMIC_DRAW) {
        glCreateBuffers(1, &m_glBuffer);
        glNamedBufferData(m_glBuffer, BufferSize, nullptr, storageFlags);
    }

    InstanceAttribBuffer(const InstanceAttribBuffer&) = delete;
    InstanceAttribBuffer(InstanceAttribBuffer&& other)
        : m_glBuffer{other.m_glBuffer} {
        other.m_glBuffer = GL_NONE;
    };

    InstanceAttribBuffer& operator=(const InstanceAttribBuffer&) = delete;
    InstanceAttribBuffer& operator=(InstanceAttribBuffer&& other) {
        if (m_glBuffer != GL_NONE) {
            glDeleteBuffers(1, &m_glBuffer);
        }
        m_glBuffer = other.m_glBuffer;
        other.m_glBuffer = GL_NONE;
        return *this;
    };

    ~InstanceAttribBuffer() {
        if (m_glBuffer != GL_NONE) {
            glDeleteBuffers(1, &m_glBuffer);
        }
    }

    void write(const std::vector<Attrib>& data) {
        if (data.size() > capacity) {
            throw std::logic_error(
                "Insufficient instance attribute buffer size");
        }
        glNamedBufferSubData(m_glBuffer, 0, sizeof(Attrib) * data.size(),
                             data.data());
    }

   private:
    template <typename, typename, size_t, size_t>
    friend class MeshRenderer;

    static const size_t capacity{Size};
    GLuint m_glBuffer;
};

template <typename VertexType, typename InstanceType>
class RenderCommand {
   public:
    RenderCommand(const std::vector<command::Draw>& commands,
                  const std::vector<GLuint>& materialIndices,
                  const std::vector<InstanceType>& instanceAttributes,
                  GLenum drawMode = GL_TRIANGLES)
        : m_commands{commands},
          m_instanceAttributes{instanceAttributes},
          m_materialIndices{materialIndices},
          m_drawMode{drawMode} {
        if (m_commands.size() != m_materialIndices.size()) {
            throw std::logic_error("Invalid render command specification");
        }
        uint32_t instanceCount{};
        for (auto& command : m_commands) {
            instanceCount += command.instanceCount;
        }
        if (instanceCount != m_instanceAttributes.size()) {
            throw std::logic_error("Insufficient instance attributes");
        }
    }

    RenderCommand(const RenderCommand&) = default;
    RenderCommand(RenderCommand&&) = default;

    RenderCommand& operator=(const RenderCommand&) = default;
    RenderCommand& operator=(RenderCommand&&) = default;

   private:
    template <typename, typename, size_t, size_t>
    friend class MeshRenderer;

    std::vector<command::Draw> m_commands;
    std::vector<InstanceType> m_instanceAttributes;
    std::vector<GLuint> m_materialIndices;

    GLenum m_drawMode;
};

template <typename VertexType, typename InstanceType,
          size_t DrawBufferSize = 32, size_t InstanceBufferSize = 128>
class MeshRenderer : public RendererState {
   public:
    MeshRenderer() {
        GLuint nextAttribIndex{0};
        glCreateVertexArrays(1, &m_glVertexArray);
        VertexAttribs<VertexType>::setup(m_glVertexArray, vertexBufferIndex,
                                         nextAttribIndex);
        InstanceAttribs<InstanceType>::setup(
            m_glVertexArray, instanceBufferIndex, nextAttribIndex);
        bindInstanceBuffer();
    };

    MeshRenderer(const MeshRenderer&) = delete;
    MeshRenderer(MeshRenderer&& other)
        : m_instanceBuffer{std::move(other.m_instanceBuffer)},
          m_drawIndirectBuffer{std::move(other.m_drawIndirectBuffer)},
          m_materialIndexBuffer{std::move(other.m_materialIndexBuffer)} {
        m_currentVertexBuffer = other.m_currentVertexBuffer;
        m_glVertexArray = other.m_glVertexArray;
        other.m_currentVertexBuffer = nullptr;
        other.m_glVertexArray = GL_NONE;
    };

    MeshRenderer& operator=(const MeshRenderer&) = delete;
    MeshRenderer& operator=(MeshRenderer&& other) {
        m_instanceBuffer = std::move(other.m_instanceBuffer);
        m_drawIndirectBuffer = std::move(other.m_drawIndirectBuffer);
        m_materialIndexBuffer = std::move(other.m_materialIndexBuffer);
        m_currentVertexBuffer = other.m_currentVertexBuffer;
        m_glVertexArray = other.m_glVertexArray;
        other.m_currentVertexBuffer = nullptr;
        other.m_glVertexArray = GL_NONE;
        return *this;
    };

    ~MeshRenderer() override {
        if (m_glVertexArray != GL_NONE) {
            if (m_glVertexArray == currentVertexArray) {
                currentVertexArray = GL_NONE;
                glBindVertexArray(GL_NONE);
            }
            glDeleteVertexArrays(1, &m_glVertexArray);
        }
    };

    void bind() {
        if (m_glVertexArray != currentVertexArray) {
            currentVertexArray = m_glVertexArray;
            glBindVertexArray(m_glVertexArray);
        }
        m_drawIndirectBuffer.bind();
    }

    RenderCommand<VertexType, InstanceType> prepareCommand(
        const Model<VertexType>& model, const InstanceType& instance) {
        size_t drawInfoCount{model.drawInfos.size()};

        std::vector<InstanceType> instanceData(drawInfoCount, instance);

        std::vector<GLuint> materialIndices{};
        materialIndices.reserve(drawInfoCount);

        std::vector<command::Draw> commands{};
        commands.reserve(drawInfoCount);

        uint32_t baseInstance{0};
        for (auto& drawInfo : model.drawInfos) {
            commands.emplace_back(drawInfo.getCommand(1, baseInstance++));
            materialIndices.emplace_back(drawInfo.materialIndex);
        }

        RenderCommand<VertexType, InstanceType> renderCommand{
            commands, materialIndices, instanceData};

        return renderCommand;
    }

    void drawSingle(const Model<VertexType>& model,
                    const InstanceType& instance) {
        size_t drawInfoCount{model.drawInfos.size()};

        std::vector<InstanceType> instanceData(drawInfoCount, instance);

        std::vector<GLuint> materialIndices{};
        materialIndices.reserve(drawInfoCount);

        std::vector<command::Draw> commands{};
        commands.reserve(drawInfoCount);

        uint32_t baseInstance{0};
        for (auto& drawInfo : model.drawInfos) {
            commands.emplace_back(drawInfo.getCommand(1, baseInstance++));
            materialIndices.emplace_back(drawInfo.materialIndex);
        }

        RenderCommand<VertexType, InstanceType> renderCommand{
            commands, materialIndices, instanceData};

        processCommand(renderCommand);
    }

    void drawInstanced(
        const std::vector<RenderCommand<VertexType, InstanceType>>& commands) {
        for (const auto& command : commands) {
            processCommand(command);
        }
    }

    void updateVertexBufferBinding(const VertexBuffer<VertexType>& buffer) {
        if (m_currentVertexBuffer != &buffer) {
            glVertexArrayVertexBuffer(m_glVertexArray, vertexBufferIndex,
                                      buffer.m_glBuffers.vertex, 0,
                                      sizeof(VertexType));
            glVertexArrayElementBuffer(m_glVertexArray,
                                       buffer.m_glBuffers.index);
            m_currentVertexBuffer = &buffer;
        }
    }

   private:
    void bindInstanceBuffer() {
        glVertexArrayBindingDivisor(m_glVertexArray, instanceBufferIndex, 1);
        glVertexArrayVertexBuffer(m_glVertexArray, instanceBufferIndex,
                                  m_instanceBuffer.m_glBuffer, 0,
                                  sizeof(InstanceType));
    };

    void processCommand(
        const RenderCommand<VertexType, InstanceType>& command) {
        m_drawIndirectBuffer.write(command.m_commands);
        m_materialIndexBuffer.write(command.m_materialIndices);
        m_instanceBuffer.write(command.m_instanceAttributes);

        glMultiDrawElementsIndirect(command.m_drawMode, GL_UNSIGNED_INT, NULL,
                                    m_drawIndirectBuffer.commandCount(),
                                    sizeof(command::Draw));
    }

    static const size_t drawBufferCapacity{DrawBufferSize};
    static const size_t instanceBufferCapacity{InstanceBufferSize};

    InstanceAttribBuffer<InstanceType, InstanceBufferSize> m_instanceBuffer;
    DrawIndicredBuffer<DrawBufferSize> m_drawIndirectBuffer;
    MaterialIndexBuffer<DrawBufferSize> m_materialIndexBuffer;

    const VertexBuffer<VertexType>* m_currentVertexBuffer;

    GLuint m_glVertexArray;
};

}  // namespace gl
