#include "world/tile_set.h"

#include <iostream>
#include <magic_enum.hpp>
#include <stdexcept>

using namespace magic_enum;
using namespace std::string_literals;

const float EPSILON = 1e-6;

namespace world {

TileSet::TileSet(const std::vector<gltf::Mesh<RigidVertex>>& tileGeometry,
                 glm::vec3 tileSize) {
    std::vector<std::array<std::vector<glm::uvec2>, 6>> tilePlanePoints{};
    std::vector<std::array<std::pair<size_t, int32_t>, 6>> tilePlaneHashes{};
    tilePlanePoints.resize(tileGeometry.size());
    tilePlaneHashes.resize(tileGeometry.size());
    for (size_t i{0}; i < tileGeometry.size(); i++) {
        processTileGeometry(tileGeometry[i], tileSize, tilePlanePoints[i],
                            tilePlaneHashes[i]);
    }
    std::unordered_map<
        int32_t,
        std::unordered_map<size_t, std::vector<std::pair<uint32_t, uint32_t>>>>
        tileMatcher{};
    for (uint32_t i{0}; i < tilePlaneHashes.size(); i++) {
        for (uint32_t f{0}; f < 6; f++) {
            auto [planeHash, symmetry] = tilePlaneHashes[i][f];
            auto& faces = tileMatcher[symmetry][planeHash];
            faces.emplace_back(i, f);
        }
    }

    m_constraints.resize(tileGeometry.size());
    for (size_t i{0}; i < tilePlaneHashes.size(); i++) {
        for (size_t f{0}; f < 6; f++) {
            auto [planeHash, symmetry] = tilePlaneHashes[i][f];
            m_constraints[i][f] = tileMatcher[symmetry * -1][planeHash];
        }
    }
};

void TileSet::processTileGeometry(
    const gltf::Mesh<RigidVertex>& tileGeometry, glm::vec3 tileSize,
    std::array<std::vector<glm::uvec2>, 6>& planeVertices,
    std::array<std::pair<size_t, int32_t>, 6>& planeHashes) {
    std::vector<glm::vec3> cleanVertices{};
    std::vector<uint32_t> cleanIndices{};

    removeDuplicates(tileGeometry, cleanVertices, cleanIndices);
    normalizeVertices(cleanVertices, tileSize);
    auto edges = findEdges(cleanIndices);

    std::array<std::unordered_map<uint32_t, uint32_t>, 6> edgeMaps{};
    getPlaneEdgeMaps(cleanVertices, edges, edgeMaps);
    for (size_t i{0}; i < 6; i++) {
        planeVertices[i] = getPlaneQuantizedVertices(edgeMaps[i], cleanVertices,
                                                     enum_value<TilePlane>(i));
        planeHashes[i] = planeHash(planeVertices[i]);
    }
}

glm::vec2 TileSet::toPlaneCoordinates(glm::vec3 normPoint, TilePlane plane) {
    switch (plane) {
        case TilePlane::PosX: {
            return glm::vec2{normPoint.y + 1.0f, normPoint.z + 1.0f};
        }
        case TilePlane::PosY: {
            return glm::vec2{1.0f - normPoint.x, normPoint.z + 1.0f};
        }
        case TilePlane::PosZ: {
            return glm::vec2{normPoint.y + 1.0f, normPoint.x + 1.0f};
        }
        case TilePlane::NegX: {
            return glm::vec2{1.0f - normPoint.y, normPoint.z + 1.0f};
        }
        case TilePlane::NegY: {
            return glm::vec2{normPoint.x + 1.0f, normPoint.z + 1.0f};
        }
        case TilePlane::NegZ: {
            return glm::vec2{1.0f - normPoint.y, normPoint.x + 1.0f};
        }
        default:
            return {0.0f, 0.0f};
    }
}

void TileSet::removeDuplicates(const gltf::Mesh<RigidVertex>& mesh,
                               std::vector<glm::vec3>& cleanVertices,
                               std::vector<uint32_t>& cleanIndices) {
    const auto& indices = mesh.indices().value();
    const auto& vertices = mesh.vertices();

    std::unordered_map<glm::vec3, uint32_t> vertMap{};
    for (size_t i{0}; i < indices.size(); i++) {
        auto vertex = vertices[indices[i]];
        auto item = vertMap.find(vertex.pos);
        if (item != vertMap.end()) {
            cleanIndices.push_back(item->second);
        } else {
            cleanVertices.push_back(vertex.pos);
            uint32_t index = static_cast<uint32_t>(cleanVertices.size() - 1);
            cleanIndices.push_back(index);
            vertMap.emplace(vertex.pos, index);
        }
    }
};

std::unordered_map<uint32_t, uint32_t> TileSet::findEdges(
    const std::vector<uint32_t>& indices) {
    std::unordered_map<uint32_t, uint32_t> edges{};

    std::unordered_set<uint64_t> encodedEdges{};
    for (size_t t{0}; t < indices.size(); t += 3) {
        for (size_t i{0}; i < 3; i++) {
            uint64_t edgeHash =
                indices[t + i] +
                (static_cast<uint64_t>(indices[t + (i + 1) % 3]) << 32);
            if (encodedEdges.count(edgeHash)) {
                encodedEdges.erase(edgeHash);
            } else {
                encodedEdges.insert(
                    indices[t + (i + 1) % 3] +
                    (static_cast<uint64_t>(indices[t + i]) << 32));
            }
        }
    }

    for (auto edgeHash : encodedEdges) {
        uint32_t end = static_cast<uint32_t>(edgeHash >> 32);
        uint32_t beg = static_cast<uint32_t>(edgeHash);
        edges.emplace(beg, end);
    }

    return edges;
}

void TileSet::normalizeVertices(std::vector<glm::vec3>& vertices,
                                glm::vec3 tileSize) {
    auto tileInvHalfDiag = glm::vec3{2.0f} / tileSize;
    for (auto& vert : vertices) {
        vert *= tileInvHalfDiag;
    }
}

void TileSet::getPlaneEdgeMaps(
    const std::vector<glm::vec3>& vertices,
    const std::unordered_map<uint32_t, uint32_t>& tileEdges,
    std::array<std::unordered_map<uint32_t, uint32_t>, 6>& edgeMaps) {
    for (auto [beg, end] : tileEdges) {
        glm::vec4 begVert{vertices[beg], 1.0f};
        glm::vec4 endVert{vertices[end], 1.0f};
        for (size_t i{0}; i < 6; i++) {
            if (glm::abs(glm::dot(normalizedPlanes[i], begVert)) < EPSILON &&
                glm::abs(glm::dot(normalizedPlanes[i], endVert)) < EPSILON) {
                edgeMaps[i].emplace(beg, end);
            }
        }
    }
}

std::vector<glm::uvec2> TileSet::getPlaneQuantizedVertices(
    std::unordered_map<uint32_t, uint32_t> edgeMap,
    const std::vector<glm::vec3>& vertices, TilePlane plane, float gridSize) {
    std::unordered_set<uint32_t> beginIndex{};
    for (auto [beg, end] : edgeMap) {
        beginIndex.insert(beg);
    }
    for (auto [beg, end] : edgeMap) {
        beginIndex.erase(end);
    }
    if (beginIndex.size() != 1) {
        return {};
    }
    uint32_t beg{*beginIndex.begin()}, end{};
    std::vector<glm::uvec2> planeVertices{};
    while (true) {
        auto vert = toPlaneCoordinates(vertices[beg], plane);
        planeVertices.push_back(glm::uvec2{glm::roundEven(vert.x / gridSize),
                                           glm::roundEven(vert.y / gridSize)});

        end = edgeMap.at(beg);
        edgeMap.erase(beg);

        auto edge = edgeMap.find(end);
        if (edge != edgeMap.end()) {
            beg = end;
        } else {
            auto vert = toPlaneCoordinates(vertices[end], plane);
            planeVertices.push_back(
                glm::uvec2{glm::roundEven(vert.x / gridSize),
                           glm::roundEven(vert.y / gridSize)});
            break;
        }
    }
    return planeVertices;
}

std::pair<size_t, int32_t> TileSet::planeHash(
    std::vector<glm::uvec2> orderedVerts, float gridSize) {
    if (orderedVerts.empty()) return {0, 0};
    if (orderedVerts.front().x > orderedVerts.back().x) {
        std::reverse(orderedVerts.begin(), orderedVerts.end());
    }

    std::vector<glm::uvec2> mirroredVerts{orderedVerts};
    uint32_t maxVal = glm::roundEven(2.0f / gridSize);
    for (auto& vert : mirroredVerts) {
        vert.x = maxVal - vert.x;
    }

    std::reverse(mirroredVerts.begin(), mirroredVerts.end());
    size_t orderedHash = std::hash<std::vector<glm::uvec2>>{}(orderedVerts);
    size_t mirroredHash = std::hash<std::vector<glm::uvec2>>{}(mirroredVerts);

    int32_t symmetry = 0;
    if (orderedHash != mirroredHash) {
        symmetry = orderedHash < mirroredHash ? 1 : -1;
    }
    size_t planeHash = orderedHash + mirroredHash;

    return {planeHash, symmetry};
}

}  // namespace world