#pragma once

#include <functional>
#include <glm/glm.hpp>
#include <glm/vector_relational.hpp>
#include <iostream>
#include <magic_enum.hpp>
#include <optional>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "graphics/gltf/document.h"
#include "graphics/vertex.h"
#include "world/tile_set.h"

class TileMap {
   public:
    TileMap(const gltf::Document& document);

    static void matchTiles(
        const std::vector<gltf::Mesh<RigidVertex>>& tileMeshes,
        glm::vec3 tileSize,
        std::vector<std::vector<gltf::Mesh<DebugVertex>>>& debugResult,
        float edgeThreshold = 1e-6) {
        auto halfDiag = glm::abs(tileSize) / 2.0f;
        for (const auto& mesh : tileMeshes) {
            debugResult.emplace_back(findEdges(mesh, halfDiag, edgeThreshold));
        }
    }

   private:
    enum class TilePlane {
        PosX,
        PosY,
        PosZ,
        NegX,
        NegY,
        NegZ,
    };

    static constexpr std::array<glm::vec4, 6> normalizedPlanes{
        glm::vec4{1.0f, 0.0f, 0.0f, 1.0f},  glm::vec4{0.0f, 1.0f, 0.0f, 1.0f},
        glm::vec4{0.0f, 0.0f, 1.0f, 1.0f},  glm::vec4{-1.0f, 0.0f, 0.0f, 1.0f},
        glm::vec4{0.0f, -1.0f, 0.0f, 1.0f}, glm::vec4{0.0f, 0.0f, -1.0f, 1.0f},
    };

    static glm::vec2 toPlaneCoordinates(glm::vec3 normPoint, TilePlane plane) {
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

    static std::pair<size_t, int32_t> planeHash(
        std::unordered_map<uint32_t, uint32_t> edges, TilePlane plane,
        const std::vector<DebugVertex>& vertices) {
        std::unordered_set<uint32_t> beginIndex{};
        for (auto [begin, end] : edges) {
            beginIndex.insert(begin);
        }
        for (auto [begin, end] : edges) {
            beginIndex.erase(end);
        }

        std::vector<glm::vec2> orderedVertices{};
        uint32_t begin{}, end{};
        if (!beginIndex.empty()) {
            begin = *beginIndex.begin();
        } else {
            begin = std::min_element(edges.begin(), edges.end(),
                                     [&](auto& a, auto& b) {
                                         return vertices[a.first].pos.y <
                                                vertices[b.first].pos.y;
                                     })
                        ->first;
        }
        while (!edges.empty()) {
            orderedVertices.push_back(
                toPlaneCoordinates(vertices[begin].pos, plane));

            end = edges[begin];
            edges.erase(begin);

            auto edge = edges.find(end);
            if (edge != edges.end()) {
                begin = end;
            } else {
                orderedVertices.push_back(
                    toPlaneCoordinates(vertices[end].pos, plane));
                break;
            }
        }

        std::vector<glm::vec2> reversedVertices{orderedVertices};
        if (orderedVertices.front().x < orderedVertices.back().x) {
            std::reverse(reversedVertices.begin(), reversedVertices.end());
        } else {
            std::reverse(orderedVertices.begin(), orderedVertices.end());
        }

        size_t orderedHash =
            std::hash<std::vector<glm::vec2>>{}(orderedVertices);
        size_t reversedHash =
            std::hash<std::vector<glm::vec2>>{}(reversedVertices);

        size_t planeHash = orderedHash ^ reversedHash;
        uint32_t winding = orderedHash < reversedHash ? 1 : -1;

        return {planeHash, winding};
    }

    static gltf::Mesh<DebugVertex> removeDuplicates(
        const gltf::Mesh<RigidVertex>& mesh) {
        const auto& indices = mesh.indices().value();
        const auto& vertices = mesh.vertices();

        std::vector<uint32_t> cleanIndices{};
        std::vector<DebugVertex> cleanVertices{};

        std::unordered_map<glm::vec3, uint32_t> vertMap{};
        for (size_t i{0}; i < indices.size(); i++) {
            auto vertex = vertices[indices[i]];
            auto item = vertMap.find(vertex.pos);
            if (item != vertMap.end()) {
                cleanIndices.push_back(item->second);
            } else {
                cleanVertices.push_back(DebugVertex{vertex.pos});
                cleanIndices.push_back(cleanVertices.size() - 1);
                vertMap.emplace(vertex.pos, cleanVertices.size() - 1);
            }
        }
        return gltf::Mesh<DebugVertex>{std::move(cleanVertices),
                                       std::move(cleanIndices),
                                       gltf::PrimitiveMode::Triangles};
    };

    static std::vector<gltf::Mesh<DebugVertex>> findEdges(
        const gltf::Mesh<RigidVertex>& mesh, glm::vec3 tileHalfDiag,
        float edgeThreshold = 1e-4) {
        if (mesh.mode() != gltf::PrimitiveMode::Triangles) {
            throw std::runtime_error("Triangle mesh required");
        }

        auto cleanMesh = removeDuplicates(mesh);

        std::unordered_set<uint64_t> edges{};
        auto& vertices = cleanMesh.vertices();
        const auto& indices = cleanMesh.indices().value();
        for (size_t t{0}; t < indices.size(); t += 3) {
            for (size_t i{0}; i < 3; i++) {
                uint64_t edgeHash =
                    indices[t + i] +
                    (static_cast<uint64_t>(indices[t + (i + 1) % 3]) << 32);
                if (edges.count(edgeHash)) {
                    edges.erase(edgeHash);
                } else {
                    edges.insert(indices[t + (i + 1) % 3] +
                                 (static_cast<uint64_t>(indices[t + i]) << 32));
                }
            }
        }

        auto tileHalfDiagInv = glm::vec3{1.0f} / tileHalfDiag;
        std::array<std::unordered_map<uint32_t, uint32_t>, 6> faceEdgeMap{};
        for (auto edge : edges) {
            uint32_t end = static_cast<uint32_t>(edge >> 32);
            uint32_t beg = static_cast<uint32_t>(edge);

            vertices[end].pos *= tileHalfDiagInv;
            vertices[beg].pos *= tileHalfDiagInv;

            auto endVert = glm::vec4{vertices[end].pos, 1.0f};
            auto begVert = glm::vec4{vertices[beg].pos, 1.0f};
            for (size_t i{0}; i < 6; i++) {
                if ((glm::dot(normalizedPlanes[i], begVert) < 1e-6) &&
                    (glm::dot(normalizedPlanes[i], endVert) < 1e-6)) {
                    faceEdgeMap[i].emplace(beg, end);
                };
            }
        }

        std::array<std::optional<std::pair<size_t, int32_t>>, 6>
            faceMatchData{};
        for (size_t i{0}; i < faceMatchData.size(); i++) {
            auto& faceEdges = faceEdgeMap[i];
            if (!faceEdges.empty()) {
                faceMatchData[i] =
                    planeHash(faceEdges, enum_value<TilePlane>(i), vertices);
            }
        }

        std::vector<gltf::Mesh<DebugVertex>> edgeMeshes{};
        for (auto& faceEdges : faceEdgeMap) {
            if (faceEdges.empty()) {
                continue;
            }
            std::vector<DebugVertex> faceVertices{};
            std::vector<uint32_t> faceIndices{};

            uint32_t begin{}, end{};
            std::unordered_set<uint32_t> edgeStart{};
            for (auto& [begin, end] : faceEdges) {
                edgeStart.insert(begin);
            }
            for (auto& [begin, end] : faceEdges) {
                edgeStart.erase(end);
            }
            if (!edgeStart.empty()) {
                begin = *edgeStart.begin();
                while (!faceEdges.empty()) {
                    faceIndices.push_back(faceVertices.size());
                    faceVertices.push_back(DebugVertex{vertices[begin].pos});

                    end = faceEdges[begin];
                    faceEdges.erase(begin);

                    auto edge = faceEdges.find(end);
                    if (edge != faceEdges.end()) {
                        begin = end;
                    } else {
                        faceIndices.push_back(faceVertices.size());
                        faceVertices.push_back(DebugVertex{vertices[end].pos});
                        if (!faceEdges.empty()) {
                            begin = faceEdges.begin()->first;
                        }
                    }
                }

                edgeMeshes.emplace_back(std::move(faceVertices),
                                        std::move(faceIndices),
                                        gltf::PrimitiveMode::LineStrip);
            }
        }
        return edgeMeshes;
    };
};
