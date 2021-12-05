#pragma once

#include <glad/glad.h>

#include <array>
#include <magic_enum.hpp>
#include <string>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <vector>

#include "graphics/gltf/mesh.h"

using namespace magic_enum;

namespace gl {

template <typename Vert>
class VertexBuffer {
   public:
    struct Buffers {
        GLuint vertex;
        GLuint index;
    };

    VertexBuffer(const std::vector<gltf::Mesh<Vert>>& resources) {
        size_t vertexBufferSize{};
        size_t indexBufferSize{};
        size_t stagingBufferSize{};
        getBuffersByteSize(resources, vertexBufferSize, indexBufferSize,
                           stagingBufferSize);
        createBuffers(resources, vertexBufferSize, indexBufferSize,
                      stagingBufferSize);
    };

    VertexBuffer& operator=(const VertexBuffer&) = delete;
    VertexBuffer& operator=(VertexBuffer&& other) {
        m_meshOffsets = std::move(other.m_meshOffsets);
        m_glBuffers = other.m_glBuffers;
        other.m_glBuffers.vertex = GL_NONE;
        other.m_glBuffers.index = GL_NONE;
    };

    VertexBuffer(const VertexBuffer&) = delete;
    VertexBuffer(VertexBuffer&& other)
        : m_meshOffsets{std::move(other.m_meshOffsets)},
          m_glBuffers{other.m_glBuffers} {
        other.m_glBuffers.vertex = GL_NONE;
        other.m_glBuffers.index = GL_NONE;
    }

    ~VertexBuffer() {
        glDeleteBuffers(2, reinterpret_cast<GLuint*>(&m_glBuffers));
    }

    struct MeshOffset {
        const uint32_t baseVertex;
        const uint32_t indexPointer;
        const uint32_t numIndices;
        const GLenum mode;
    };

    MeshOffset getOffset(size_t index) const noexcept {
        return m_meshOffsets[index];
    }

   private:
    template <typename, typename, size_t, size_t>
    friend class Renderer;

    void createBuffers(const std::vector<gltf::Mesh<Vert>>& meshes,
                       size_t vertexBufferSize, size_t indexBufferSize,
                       size_t stagingBufferSize) {
        glCreateBuffers(2, reinterpret_cast<GLuint*>(&m_glBuffers));

        glNamedBufferStorage(m_glBuffers.vertex,
                             vertexBufferSize * sizeof(Vert), NULL, GL_NONE);
        glNamedBufferStorage(m_glBuffers.index,
                             indexBufferSize * sizeof(uint32_t), NULL, GL_NONE);

        GLuint stagingBuffer{};
        glCreateBuffers(1, &stagingBuffer);
        glNamedBufferStorage(stagingBuffer, stagingBufferSize, NULL,
                             GL_DYNAMIC_STORAGE_BIT);

        auto u32Checked = [](size_t size) {
            uint32_t u32{static_cast<uint32_t>(size)};
            if (u32 < size) {
                throw std::runtime_error("Unsigned overflow");
            }
            return u32;
        };

        size_t vertexOffset{0};
        size_t indexOffset{0};

        size_t baseVertex{0};
        for (const auto& mesh : meshes) {
            const auto& vertices = mesh.vertices();
            const auto& indices = mesh.indices();

            size_t vertexByteSize = vertices.size() * sizeof(Vert);
            glNamedBufferSubData(stagingBuffer, 0, vertexByteSize,
                                 vertices.data());

            glCopyNamedBufferSubData(stagingBuffer, m_glBuffers.vertex, 0,
                                     vertexOffset, vertexByteSize);

            size_t indexByteSize{0};
            size_t indexCount{0};
            if (indices.has_value()) {
                indexCount = indices.value().size();
                indexByteSize = indices.value().size() * sizeof(uint32_t);
                glNamedBufferSubData(stagingBuffer, 0, indexByteSize,
                                     indices.value().data());
                glCopyNamedBufferSubData(stagingBuffer, m_glBuffers.index, 0,
                                         indexOffset, indexByteSize);
            }
            m_meshOffsets.push_back(
                MeshOffset{u32Checked(baseVertex),
                           u32Checked(indexOffset / sizeof(uint32_t)),
                           u32Checked(indexCount), enum_integer(mesh.mode())});

            baseVertex += vertices.size();
            indexOffset += indexByteSize;
            vertexOffset += vertexByteSize;
        }
        glDeleteBuffers(1, &stagingBuffer);
    };

    void getBuffersByteSize(const std::vector<gltf::Mesh<Vert>>& meshes,
                            size_t& vertexBufferSize, size_t& indexBufferSize,
                            size_t& stagingBufferSize) {
        size_t maxSize{0};
        for (const auto& mesh : meshes) {
            const auto& vertices = mesh.vertices();
            const auto& indices = mesh.indices();

            if (indices.has_value()) {
                indexBufferSize += indices.value().size();
                maxSize = std::max(maxSize,
                                   indices.value().size() * sizeof(uint32_t));
            } else {
                throw std::invalid_argument(
                    "Mesh does not provide index buffer");
            }
            vertexBufferSize += vertices.size();
            maxSize = std::max(maxSize, vertices.size() * sizeof(Vert));
        }
        stagingBufferSize = maxSize;
    };

    std::vector<MeshOffset> m_meshOffsets;
    Buffers m_glBuffers;
};
}  // namespace gl