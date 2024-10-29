#pragma once

#include <fx/gltf.h>

#include <array>
#include <glm/glm.hpp>
#include <magic_enum.hpp>
#include <optional>
#include <string>
#include <unordered_set>

using namespace magic_enum;

namespace gltf {

class pbrMaterial {
   public:
    enum class TextureMap : uint8_t {
        Color,
        MetallicRoughness,
        Normal,
        Occlusion,
        Emission
    };

    pbrMaterial() = default;

    pbrMaterial(const pbrMaterial&) = default;
    pbrMaterial(pbrMaterial&&) = default;

    pbrMaterial& operator=(const pbrMaterial&) = default;
    pbrMaterial& operator=(pbrMaterial&&) = default;

    pbrMaterial(const std::filesystem::path& path,
                const fx::gltf::Document& document,
                const fx::gltf::Material& material);

    static pbrMaterial null() {
        pbrMaterial null{};
        null.m_baseColor = glm::vec4{0.85f, 0.15f, 0.5f, 1.0f};
        null.m_emissionStrength = glm::vec3{0.0f};
        null.m_metalness = 0.0f;
        null.m_normalScale = 1.0f;
        null.m_occlusionStrength = 1.0f;
        null.m_roughness = 0.8f;
        return null;
    }

    std::optional<size_t> textureMap(TextureMap type) const {
        return m_textureMaps[enum_integer(type)];
    }
    const glm::vec4& baseColor() const noexcept { return m_baseColor; }
    const glm::vec3& emissionStrength() const noexcept {
        return m_emissionStrength;
    }
    float roughness() const noexcept { return m_roughness; }
    float metalness() const noexcept { return m_metalness; }
    float normalScale() const noexcept { return m_normalScale; }
    float occlusionStrength() const noexcept { return m_occlusionStrength; }

   private:
    friend class Scene;

    static std::string getGltfUID(const std::filesystem::path& path,
                                  const fx::gltf::Document& document,
                                  size_t index);
    static void getGltfTextureIndices(const fx::gltf::Document& document,
                                      const fx::gltf::Material& material,
                                      std::unordered_set<size_t>& indices);

    std::array<std::optional<size_t>, enum_count<TextureMap>()> m_textureMaps;
    glm::vec4 m_baseColor{1.0f, 1.0f, 1.0f, 1.0f};
    glm::vec3 m_emissionStrength{1.0f, 1.0f, 1.0f};
    float m_roughness{1.0f};
    float m_metalness{1.0f};
    float m_normalScale{1.0f};
    float m_occlusionStrength{1.0f};
};

}  // namespace gltf