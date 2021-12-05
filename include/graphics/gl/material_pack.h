#pragma once

#include <glad/glad.h>

#include <array>
#include <glm/glm.hpp>
#include <unordered_set>
#include <vector>

#include "graphics/gl/block/std140.h"
#include "graphics/gl/shader.h"
#include "graphics/gl/texture.h"
#include "graphics/gltf/material.h"
#include "magic_enum.hpp"

const size_t MATERIAL_PACK_SIZE = 16;

using namespace magic_enum;
using namespace std::string_literals;

namespace gl {

class Context;

namespace handle {

class MaterialPack {
   public:
    static MaterialPack null() {
        return MaterialPack(std::numeric_limits<size_t>::max());
    }

    MaterialPack(const MaterialPack&) = default;
    MaterialPack(MaterialPack&&) = default;

    MaterialPack& operator=(const MaterialPack&) = default;
    MaterialPack& operator=(MaterialPack&&) = default;

    bool operator==(const MaterialPack& rhs) const {
        return index == rhs.index;
    }
    bool operator!=(const MaterialPack& rhs) const {
        return index != rhs.index;
    }

   private:
    friend gl::Context;
    MaterialPack(size_t index) : index{index} {};

    size_t index;
};

}  // namespace handle

template <size_t>
class pbrMaterialPack;

using MaterialUniform =
    std140::Block<glm::vec4, glm::vec3, GLuint64, GLuint64, GLuint64, GLuint64,
                  GLuint64, float, float, float, float>;

template <size_t size>
class pbrMaterialPackBuilder {
   public:
    pbrMaterialPackBuilder() = default;

    pbrMaterialPackBuilder(const pbrMaterialPackBuilder&) = delete;
    pbrMaterialPackBuilder(pbrMaterialPackBuilder&&) = default;

    pbrMaterialPackBuilder& operator=(const pbrMaterialPackBuilder&) = delete;
    pbrMaterialPackBuilder& operator=(pbrMaterialPackBuilder&&) = delete;

    void setMaterialData(uint32_t documentMaterialIndex,
                         const gltf::pbrMaterial& material,
                         size_t textureIndexOffset,
                         const std::vector<Texture>& textures,
                         const Texture& null) {
        if (full()) {
            throw std::logic_error("Material pack index overflow");
        }
        auto index = m_nextFree++;

        auto getTexture = [&](gltf::pbrMaterial::TextureMap map) {
            auto textureIndex = material.textureMap(map);
            if (textureIndex.has_value()) {
                return textures[textureIndexOffset + textureIndex.value()]
                    .handle();
            } else {
                return null.handle();
            }
        };

        auto color = getTexture(gltf::pbrMaterial::TextureMap::Color);
        auto metallicRoughness =
            getTexture(gltf::pbrMaterial::TextureMap::MetallicRoughness);
        auto normal = getTexture(gltf::pbrMaterial::TextureMap::Normal);
        auto occlusion = getTexture(gltf::pbrMaterial::TextureMap::Occlusion);
        auto emission = getTexture(gltf::pbrMaterial::TextureMap::Emission);

        auto& materialBlock = gl::std140::get<0>(m_block)[index];
        gl::std140::get<0>(materialBlock) = material.baseColor();
        gl::std140::get<1>(materialBlock) = material.emissionStrength();
        gl::std140::get<2>(materialBlock) = color;
        gl::std140::get<3>(materialBlock) = metallicRoughness;
        gl::std140::get<4>(materialBlock) = normal;
        gl::std140::get<5>(materialBlock) = occlusion;
        gl::std140::get<6>(materialBlock) = emission;
        gl::std140::get<7>(materialBlock) = material.roughness();
        gl::std140::get<8>(materialBlock) = material.metalness();
        gl::std140::get<9>(materialBlock) = material.normalScale();
        gl::std140::get<10>(materialBlock) = material.occlusionStrength();

        std::unordered_set<GLuint64> uniqueTextureHandles{
            {color, metallicRoughness, normal, occlusion, emission}};
        for (auto& handle : m_textureHandles) {
            uniqueTextureHandles.erase(handle);
        }
        std::copy(uniqueTextureHandles.begin(), uniqueTextureHandles.end(),
                  std::back_inserter(m_textureHandles));
        m_materialIndices[documentMaterialIndex] = index;
    }

    bool full() const { return m_nextFree >= size; }

   private:
    friend class pbrMaterialPack<size>;

    std140::Block<MaterialUniform[size]> m_block;

    std::unordered_map<uint32_t, uint32_t> m_materialIndices;
    std::vector<GLuint64> m_textureHandles;

    uint32_t m_nextFree{};
};

template <size_t size>
class pbrMaterialPack : public UniformBlock {
   public:
    pbrMaterialPack() = default;

    pbrMaterialPack(const pbrMaterialPack&) = delete;
    pbrMaterialPack(pbrMaterialPack&& other)
        : m_materialIndices{std::move(other.m_materialIndices)},
          m_textureHandles{std::move(other.m_textureHandles)},
          m_glUniform{other.m_glUniform} {
        other.m_glUniform = GL_NONE;
    }

    pbrMaterialPack& operator=(const pbrMaterialPack&) = delete;
    pbrMaterialPack& operator=(pbrMaterialPack&& other) {
        m_materialIndices = std::move(other.m_materialIndices);
        m_textureHandles = std::move(other.m_textureHandles);
        m_glUniform = other.m_glUniform;
        other.m_glUniform = GL_NONE;
        return *this;
    };

    ~pbrMaterialPack() {
        if (m_glUniform != GL_NONE) {
            setTexturesNonResident();
            glDeleteBuffers(1, &m_glUniform);
        }
    }

    void setTexturesResident() {
        for (GLuint64 handle : m_textureHandles) {
            Texture::makeResident(handle);
        }
    }
    void setTexturesNonResident() {
        for (GLuint64 handle : m_textureHandles) {
            Texture::makeNonResident(handle);
        }
    }

    uint32_t getMaterialIndex(uint32_t documentMaterialIndex) const {
        return m_materialIndices.at(documentMaterialIndex);
    }

   private:
    friend class Context;

    const std::string& blockName() const override { return name; };
    GLuint glBuffer() const override { return m_glUniform; };

    pbrMaterialPack(pbrMaterialPackBuilder<size>&& builder)
        : m_materialIndices{std::move(builder.m_materialIndices)},
          m_textureHandles{std::move(builder.m_textureHandles)} {
        glCreateBuffers(1, &m_glUniform);
        glNamedBufferStorage(m_glUniform, sizeof(builder.m_block),
                             &builder.m_block, GL_NONE);
    }

    inline static const std::string name{"pbrMaterialPack"s +
                                         std::to_string(size)};

    std::unordered_map<uint32_t, uint32_t> m_materialIndices;
    std::vector<GLuint64> m_textureHandles;
    GLuint m_glUniform{GL_NONE};
};

using PackBuilder = pbrMaterialPackBuilder<MATERIAL_PACK_SIZE>;
using MaterialPack = pbrMaterialPack<MATERIAL_PACK_SIZE>;

}  // namespace gl
