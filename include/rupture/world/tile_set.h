#pragma once

#include <array>
#include <glm/glm.hpp>
#include <glm/vector_relational.hpp>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "rupture/graphics/gltf/mesh.h"
#include "rupture/graphics/vertex.h"

template <typename Type, typename... Rest>
void hash_combine(size_t& seed, const Type& type, const Rest&... rest) {
    seed ^= std::hash<Type>{}(type) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    (hash_combine(seed, rest), ...);
}

namespace std {

template <>
struct hash<glm::vec3> {
    size_t operator()(const glm::vec3& value) const {
        size_t seed{0};
        hash_combine(seed, value.x, value.y, value.z);
        return seed;
    }
};

template <>
struct hash<glm::uvec2> {
    size_t operator()(const glm::uvec2& value) const {
        size_t seed{0};
        hash_combine(seed, value.x, value.y);
        return seed;
    }
};

template <>
struct hash<std::vector<glm::uvec2>> {
    size_t operator()(const std::vector<glm::uvec2>& value) const {
        size_t seed{0};
        for (auto& v : value) {
            size_t vecSeed{0};
            hash_combine(vecSeed, v.x, v.y);
            hash_combine(seed, vecSeed);
        }
        return seed;
    }
};

template <>
struct hash<glm::vec2> {
    size_t operator()(const glm::vec2& value) const {
        size_t seed{0};
        hash_combine(seed, value.x, value.y);
        return seed;
    }
};

template <>
struct hash<std::vector<glm::vec2>> {
    size_t operator()(const std::vector<glm::vec2>& value) const {
        size_t seed{0};
        for (auto& v : value) {
            size_t vecSeed{0};
            hash_combine(vecSeed, v.x, v.y);
            hash_combine(seed, vecSeed);
        }
        return seed;
    }
};

template <>
struct equal_to<glm::vec3> {
    static constexpr float EPSILON = 1e-6;
    bool operator()(const glm::vec3& lhs, const glm::vec3& rhs) const {
        return glm::all(glm::epsilonEqual(lhs, rhs, glm::vec3{EPSILON}));
    }
};

}  // namespace std

namespace world {

class TileSet {
   public:
    using PlacementConstraints =
        std::vector<std::array<std::vector<std::pair<uint32_t, uint32_t>>, 6>>;

    TileSet(const std::vector<gltf::Mesh<RigidVertex>>& tileGeometry,
            glm::vec3 tileSize);

    const PlacementConstraints& placementConstraints() const {
        return m_constraints;
    }

   private:
    enum class TilePlane {
        PosX,
        PosY,
        NegX,
        NegY,
        PosZ,
        NegZ,
    };

    static constexpr std::array<glm::vec4, 6> normalizedPlanes{
        glm::vec4{1.0f, 0.0f, 0.0f, 1.0f},  glm::vec4{0.0f, 1.0f, 0.0f, 1.0f},
        glm::vec4{-1.0f, 0.0f, 0.0f, 1.0f}, glm::vec4{0.0f, -1.0f, 0.0f, 1.0f},
        glm::vec4{0.0f, 0.0f, 1.0f, 1.0f},  glm::vec4{0.0f, 0.0f, -1.0f, 1.0f},
    };

    static void processTileGeometry(
        const gltf::Mesh<RigidVertex>& tileGeometry, glm::vec3 tileSize,
        std::array<std::vector<glm::uvec2>, 6>& planeVertices,
        std::array<std::pair<size_t, int32_t>, 6>& planeHashes);

    static glm::vec2 toPlaneCoordinates(glm::vec3 normPoint, TilePlane plane);

    static void removeDuplicates(const gltf::Mesh<RigidVertex>& mesh,
                                 std::vector<glm::vec3>& cleanVertices,
                                 std::vector<uint32_t>& cleanIndices);

    static std::unordered_map<uint32_t, uint32_t> findEdges(
        const std::vector<uint32_t>& indices);

    static void normalizeVertices(std::vector<glm::vec3>& vertices,
                                  glm::vec3 tileSize);

    static void getPlaneEdgeMaps(
        const std::vector<glm::vec3>& vertices,
        const std::unordered_map<uint32_t, uint32_t>& tileEdges,
        std::array<std::unordered_map<uint32_t, uint32_t>, 6>& edgeMaps);

    static std::vector<glm::uvec2> getPlaneQuantizedVertices(
        std::unordered_map<uint32_t, uint32_t> edgeMap,
        const std::vector<glm::vec3>& vertices, TilePlane plane,
        float gridSize = 1e-6);

    static std::pair<size_t, int32_t> planeHash(
        std::vector<glm::uvec2> orderedVerts, float gridSize = 1e-6);

    PlacementConstraints m_constraints;
};

}  // namespace world