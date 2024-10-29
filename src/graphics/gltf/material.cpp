#include "rupture/graphics/gltf/material.h"

#include "rupture/graphics/gltf/texture.h"

using namespace std::string_literals;

namespace gltf {

pbrMaterial::pbrMaterial(const std::filesystem::path& path,
                         const fx::gltf::Document& document,
                         const fx::gltf::Material& material) {
    const auto& pbr = material.pbrMetallicRoughness;

    auto getTexture = [&](const fx::gltf::Material::Texture& texture)
        -> std::optional<size_t> {
        if (!texture.empty()) {
            return document.textures.at(texture.index).source;
        } else {
            return std::nullopt;
        }
    };

    m_textureMaps[enum_integer(TextureMap::Color)] =
        getTexture(pbr.baseColorTexture);
    m_textureMaps[enum_integer(TextureMap::MetallicRoughness)] =
        getTexture(pbr.metallicRoughnessTexture);
    m_textureMaps[enum_integer(TextureMap::Normal)] =
        getTexture(material.normalTexture);
    m_textureMaps[enum_integer(TextureMap::Occlusion)] =
        getTexture(material.occlusionTexture);
    m_textureMaps[enum_integer(TextureMap::Emission)] =
        getTexture(material.emissiveTexture);

    m_baseColor = glm::vec4{pbr.baseColorFactor[0], pbr.baseColorFactor[1],
                            pbr.baseColorFactor[2], pbr.baseColorFactor[3]};
    m_emissionStrength =
        glm::vec3{material.emissiveFactor[0], material.emissiveFactor[1],
                  material.emissiveFactor[2]};
    m_roughness = pbr.roughnessFactor;
    m_metalness = pbr.metallicFactor;
    m_occlusionStrength = material.occlusionTexture.strength;
    m_normalScale = material.normalTexture.scale;
}

std::string pbrMaterial::getGltfUID(const std::filesystem::path& path,
                                    const fx::gltf::Document& document,
                                    size_t index) {
    const auto& material = document.materials.at(index);
    if (!material.name.empty()) {
        return path.stem().string() + "." + material.name;
    } else {
        return path.stem().string() + ".Material." + std::to_string(index);
    }
}

void pbrMaterial::getGltfTextureIndices(const fx::gltf::Document& document,
                                        const fx::gltf::Material& material,
                                        std::unordered_set<size_t>& indices) {
    const auto& pbr = material.pbrMetallicRoughness;
    if (!pbr.baseColorTexture.empty()) {
        indices.insert(document.textures.at(pbr.baseColorTexture.index).source);
    }
    if (!pbr.metallicRoughnessTexture.empty()) {
        indices.insert(
            document.textures.at(pbr.metallicRoughnessTexture.index).source);
    }
    if (!material.normalTexture.empty()) {
        indices.insert(
            document.textures.at(material.normalTexture.index).source);
    }
    if (!material.occlusionTexture.empty()) {
        indices.insert(
            document.textures.at(material.occlusionTexture.index).source);
    }
    if (!material.emissiveTexture.empty()) {
        indices.insert(
            document.textures.at(material.emissiveTexture.index).source);
    }
}

}  // namespace gltf