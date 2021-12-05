#pragma once

#include <glad/glad.h>

#include <vector>

#include "graphics/gl/material_pack.h"

namespace gl {

class Context;

namespace command {

struct Draw {
    Draw(uint32_t numIndices, uint32_t instanceCount, uint32_t firstIndex,
         uint32_t baseVertex, uint32_t baseInstance)
        : numIndices{numIndices},
          instanceCount{instanceCount},
          firstIndex{firstIndex},
          baseVertex{baseVertex},
          baseInstance{baseInstance} {}

    const uint32_t numIndices;
    const uint32_t instanceCount;
    const uint32_t firstIndex;
    const uint32_t baseVertex;
    const uint32_t baseInstance;
};

}  // namespace command

namespace handle {

template <typename Vert>
class VertexBuffer {
   public:
    VertexBuffer(const VertexBuffer&) = default;
    VertexBuffer(VertexBuffer&&) = default;

    VertexBuffer& operator=(const VertexBuffer&) = default;
    VertexBuffer& operator=(VertexBuffer&&) = default;

    bool operator==(const VertexBuffer& rhs) const {
        return index == rhs.index;
    }
    bool operator!=(const VertexBuffer& rhs) const {
        return index != rhs.index;
    }

   private:
    friend gl::Context;

    VertexBuffer(uint32_t index) : index{index} {};
    uint32_t index;
};

template <typename Vert>
class Model {
   public:
    static Model null() { return {std::numeric_limits<size_t>::max()}; }

    Model(const Model&) = default;
    Model(Model&&) = default;

    Model& operator=(const Model&) = default;
    Model& operator=(Model&&) = default;

    bool operator==(const Model& rhs) const { return index == rhs.index; }
    bool operator!=(const Model& rhs) const { return index != rhs.index; }

   private:
    friend gl::Context;
    Model(size_t index) : index{index} {};

    size_t index;
};

}  // namespace handle

template <typename Vert>
struct DrawInfo {
    command::Draw getCommand(uint32_t instanceCount = 1,
                             uint32_t baseInstance = 0) const {
        return {numIndices, instanceCount, baseIndex, baseVertex, baseInstance};
    }
    const handle::VertexBuffer<Vert> vertexBuffer;
    const handle::MaterialPack materialPack;
    const uint32_t materialIndex;
    const uint32_t baseVertex;
    const uint32_t baseIndex;
    const uint32_t numIndices;
    const GLenum drawMode;

   private:
    friend gl::Context;

    DrawInfo(handle::VertexBuffer<Vert> vertexBuffer,
             handle::MaterialPack materialPack, uint32_t materialIndex,
             uint32_t baseVertex, uint32_t baseIndex, uint32_t numIndices,
             GLenum drawMode)
        : vertexBuffer{vertexBuffer},
          materialPack{materialPack},
          materialIndex{materialIndex},
          baseVertex{baseVertex},
          baseIndex{baseIndex},
          numIndices{numIndices},
          drawMode{drawMode} {}
};

template <typename Vert>
struct Model {
    std::vector<DrawInfo<Vert>> drawInfos;
};

}  // namespace gl