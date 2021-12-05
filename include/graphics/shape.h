#pragma once

#define GLM_FORCE_RADIANS

#include <glm/glm.hpp>
#include <vector>

#include "vertex.h"

struct DebugShape {
    static DebugShape UnitCube() {
        std::vector<DebugVertex> vertices{
            DebugVertex{glm::vec3{-0.5f, -0.5f, -0.5f}},
            DebugVertex{glm::vec3{0.5f, -0.5f, -0.5f}},
            DebugVertex{glm::vec3{-0.5f, 0.5f, -0.5f}},
            DebugVertex{glm::vec3{0.5f, 0.5f, -0.5f}},
            DebugVertex{glm::vec3{-0.5f, -0.5f, 0.5f}},
            DebugVertex{glm::vec3{0.5f, -0.5f, 0.5f}},
            DebugVertex{glm::vec3{-0.5f, 0.5f, 0.5f}},
            DebugVertex{glm::vec3{0.5f, 0.5f, 0.5f}},
        };
        std::vector<uint32_t> indices{0, 1, 0, 2, 1, 3, 2, 3, 4, 5, 4, 6,
                                      5, 7, 6, 7, 0, 4, 1, 5, 2, 6, 3, 7};
        return {indices, vertices};
    }

    std::vector<uint32_t> indices;
    std::vector<DebugVertex> vertices;
};

struct Shape {
    static Shape Cube(glm::vec3 minBound, glm::vec3 maxBound) {
        auto cube = tessellatedCube(0);
        auto extent = maxBound - minBound;
        for (auto& vert : cube.vertices) {
            vert.pos = vert.pos * extent + minBound;
        }
        return cube;
    }
    static Shape Sphere(float radius) {
        auto sphere = tessellatedCube(5);
        for (auto& vert : sphere.vertices) {
            vert.pos = glm::normalize(vert.pos - 0.5f) * radius;
        }
        return sphere;
    }

    std::vector<uint32_t> indices;
    std::vector<RigidVertex> vertices;

   private:
    static Shape tessellatedCube(uint32_t subdivLevel) {
        uint32_t faceVertices = (subdivLevel + 2) * (subdivLevel + 2);
        uint32_t faceIndices = (subdivLevel + 1) * (subdivLevel + 1) * 6;
        std::vector<RigidVertex> vertices(faceVertices * 6);
        std::vector<uint32_t> indices(faceIndices * 6);

        auto fillFace = [&](uint32_t baseVertex, uint32_t baseIndex,
                            glm::vec3 baseOffset, glm::vec3 iStride,
                            glm::vec3 jStride) mutable {
            auto norm = glm::cross(iStride, jStride);
            for (uint32_t i{0}; i < subdivLevel + 2; i++) {
                for (uint32_t j{0}; j < subdivLevel + 2; j++) {
                    auto& vert =
                        vertices[baseVertex + i * (subdivLevel + 2) + j];
                    vert.pos = baseOffset + static_cast<float>(i) * iStride +
                               static_cast<float>(j) * jStride;
                    vert.tex =
                        glm::vec2{static_cast<float>(i) / (subdivLevel + 1),
                                  static_cast<float>(j) / (subdivLevel + 1)};
                    vert.norm = norm;
                }
            }
            for (uint32_t f{0}; f < (subdivLevel + 1) * (subdivLevel + 1);
                 f++) {
                uint32_t i = f / (subdivLevel + 1);
                uint32_t j = f % (subdivLevel + 1);
                indices[baseIndex + f + 0] =
                    (baseIndex + (i + 0) * (subdivLevel + 2) + (j + 0));
                indices[baseIndex + f + 1] =
                    (baseIndex + (i + 0) * (subdivLevel + 2) + (j + 1));
                indices[baseIndex + f + 2] =
                    (baseIndex + (i + 1) * (subdivLevel + 2) + (j + 0));
                indices[baseIndex + f + 3] =
                    (baseIndex + (i + 1) * (subdivLevel + 2) + (j + 0));
                indices[baseIndex + f + 4] =
                    (baseIndex + (i + 0) * (subdivLevel + 2) + (j + 1));
                indices[baseIndex + f + 5] =
                    (baseIndex + (i + 1) * (subdivLevel + 2) + (j + 1));
            }
        };

        float vertexStride = 1.0f / (subdivLevel + 1);
        fillFace(0 * faceVertices, 0 * faceIndices, glm::vec3{0, 0, 0},
                 glm::vec3{vertexStride, 0.0f, 0.0f},
                 glm::vec3{0.0f, vertexStride, 0.0f});
        fillFace(1 * faceVertices, 1 * faceIndices, glm::vec3{0, 0, 0},
                 glm::vec3{0.0f, 0.0f, vertexStride},
                 glm::vec3{vertexStride, 0.0f, 0.0f});
        fillFace(2 * faceVertices, 2 * faceIndices, glm::vec3{0, 0, 0},
                 glm::vec3{0.0f, vertexStride, 0.0f},
                 glm::vec3{0.0f, 0.0f, vertexStride});
        fillFace(3 * faceVertices, 3 * faceIndices, glm::vec3{1.0f, 1.0f, 1.0f},
                 glm::vec3{0.0f, -vertexStride, 0.0f},
                 glm::vec3{-vertexStride, 0.0f, 0.0f});
        fillFace(4 * faceVertices, 4 * faceIndices, glm::vec3{1.0f, 1.0f, 1.0f},
                 glm::vec3{-vertexStride, 0.0f, 0.0f},
                 glm::vec3{0.0f, 0.0f, -vertexStride});
        fillFace(5 * faceVertices, 5 * faceIndices, glm::vec3{1.0f, 1.0f, 1.0f},
                 glm::vec3{0.0f, 0.0f, -vertexStride},
                 glm::vec3{0.0f, -vertexStride, 0.0f});

        return {indices, vertices};
    };
};