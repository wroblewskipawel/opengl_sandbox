#pragma once

#include "rupture/graphics/gl/light.h"
#include "rupture/graphics/gl/material_pack.h"
#include "rupture/graphics/gl/renderer/mesh.h"
#include "rupture/typemap.h"

namespace gl {
using MeshRenderers = TypeMap<
    MeshRenderer<RigidVertex, glm::mat4>, MeshRenderer<RigidVertex, glm::vec3>,
    MeshRenderer<SkinVertex, glm::mat4>, MeshRenderer<glm::vec3, DebugInstance>,
    MeshRenderer<DebugVertex, DebugInstance>>;

template <typename Vert>
using CommandIndex =
    std::pair<handle::VertexBuffer<Vert>, handle::MaterialPack>;

template <typename Vert, typename Instance>
using CommandBuffer = std::vector<RenderCommand<Vert, Instance>>;

template <typename Vert, typename Instance>
using CommandBufferMap =
    std::unordered_map<CommandIndex<Vert>, CommandBuffer<Vert, Instance>>;

using Commands = StaticTypeMap<CommandBufferMap<RigidVertex, glm::mat4>,
                               CommandBufferMap<RigidVertex, glm::vec3>,
                               CommandBufferMap<SkinVertex, glm::mat4>,
                               CommandBufferMap<glm::vec3, DebugInstance>,
                               CommandBufferMap<DebugVertex, DebugInstance>>;

using ResourcesStorage =
    TypeMap<std::vector<Shader>, std::vector<Texture>, std::vector<Environment>,
            std::vector<MaterialPack>, std::vector<LightPack>,
            std::vector<VertexBuffer<RigidVertex>>,
            std::vector<VertexBuffer<SkinVertex>>,
            std::vector<VertexBuffer<DebugVertex>>,
            std::vector<VertexBuffer<glm::vec3>>,
            std::vector<Model<RigidVertex>>, std::vector<Model<SkinVertex>>,
            std::vector<Model<DebugVertex>>, std::vector<Model<glm::vec3>>>;

template <typename Resource>
using NamedResourceMap = std::unordered_map<std::string, Resource>;

using NamedHandleMap = TypeMap<NamedResourceMap<handle::Model<RigidVertex>>,
                               NamedResourceMap<handle::Model<SkinVertex>>,
                               NamedResourceMap<handle::Model<DebugVertex>>,
                               NamedResourceMap<handle::Model<glm::vec3>>,
                               NamedResourceMap<handle::Shader>,
                               NamedResourceMap<handle::Environment>,
                               NamedResourceMap<handle::Lighting>>;

}  // namespace gl