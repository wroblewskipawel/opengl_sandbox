#pragma once

#include <fx/gltf.h>

#include <filesystem>
#include <optional>
#include <unordered_map>
#include <vector>

#include "rupture/graphics/gltf/animation.h"
#include "rupture/graphics/gltf/material.h"
#include "rupture/graphics/gltf/mesh.h"
#include "rupture/graphics/gltf/skin.h"
#include "rupture/graphics/gltf/texture.h"
#include "rupture/graphics/vertex.h"
#include "rupture/typemap.h"

namespace gltf {

class Document {
   public:
    Document(const std::filesystem::path& path);

    Document(const Document&) = delete;
    Document(Document&&) = delete;

    Document& operator=(const Document&) = delete;
    Document& operator=(Document&&) = delete;

    template <typename Resource>
    const std::vector<Resource>& at() const {
        return m_resources.at<std::vector<Resource>>();
    }

    template <typename Resource>
    std::vector<Resource>& at() {
        return m_resources.at<std::vector<Resource>>();
    }

    struct Primitive {
        uint32_t meshIndex;
        std::optional<uint32_t> materialIndex;
    };

    template <typename Vert>
    struct Model {
        std::vector<Primitive> primitives;
        std::optional<uint32_t> skin;
    };

    template <typename Vert>
    const std::unordered_map<std::string, Model<Vert>>& getModels() const {
        return m_models.at<std::unordered_map<std::string, Model<Vert>>>();
    }

    template <typename Vert>
    std::unordered_map<std::string, Model<Vert>>& getModels() {
        return m_models.at<std::unordered_map<std::string, Model<Vert>>>();
    }

    const std::string& name() const { return m_name; }

   private:
    void loadDocument(const std::filesystem::path& path,
                      const fx::gltf::Document& document);
    void loadMaterials(const std::filesystem::path& path,
                       const fx::gltf::Document& document);
    void loadAnimations(const fx::gltf::Document& document,
                        std::unordered_map<uint32_t, uint32_t>& skinMap);

    void loadMeshes(const fx::gltf::Document& document,
                    const std::unordered_map<uint32_t, uint32_t>& skinMap);

    TypeMap<std::vector<Mesh<RigidVertex>>, std::vector<Mesh<SkinVertex>>,
            std::vector<Texture>, std::vector<pbrMaterial>, std::vector<Skin>>
        m_resources;

    TypeMap<std::unordered_map<std::string, Model<SkinVertex>>,
            std::unordered_map<std::string, Model<RigidVertex>>>
        m_models;

    std::string m_name;
};

}  // namespace gltf
