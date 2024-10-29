#pragma once

#include <glad/glad.h>

#include <glm/glm.hpp>
#include <stdexcept>
#include <string>

#include "rupture/graphics/vertex.h"

using namespace std::string_literals;

namespace gl {

template <typename Vert>
struct VertexAttribs {
    static void setup(GLuint vertexArray, GLuint bufferIndex,
                      GLuint& nextIndex) {
        throw std::logic_error("Vertex layout definition not provided"s);
    }
};

template <typename Instance>
struct InstanceAttribs {
    static void setup(GLuint vertexArray, GLuint bufferIndex,
                      GLuint& nextIndex) {
        throw std::logic_error("Instance layout definition not provided"s);
    }
};

template <>
struct VertexAttribs<glm::vec3> {
    static void setup(GLuint vertexArray, GLuint bufferIndex,
                      GLuint& nextIndex) {
        glVertexArrayAttribBinding(vertexArray, nextIndex, bufferIndex);
        glVertexArrayAttribFormat(vertexArray, nextIndex, 3, GL_FLOAT, GL_FALSE,
                                  0);
        glEnableVertexArrayAttrib(vertexArray, nextIndex++);
    };
};

template <>
struct VertexAttribs<RigidVertex> {
    static void setup(GLuint vertexArray, GLuint bufferIndex,
                      GLuint& nextIndex) {
        glVertexArrayAttribBinding(vertexArray, nextIndex, bufferIndex);
        glVertexArrayAttribFormat(vertexArray, nextIndex, 3, GL_FLOAT, GL_FALSE,
                                  offsetof(RigidVertex, pos));
        glEnableVertexArrayAttrib(vertexArray, nextIndex++);

        glVertexArrayAttribBinding(vertexArray, nextIndex, bufferIndex);
        glVertexArrayAttribFormat(vertexArray, nextIndex, 3, GL_FLOAT, GL_FALSE,
                                  offsetof(RigidVertex, norm));
        glEnableVertexArrayAttrib(vertexArray, nextIndex++);

        glVertexArrayAttribBinding(vertexArray, nextIndex, bufferIndex);
        glVertexArrayAttribFormat(vertexArray, nextIndex, 4, GL_FLOAT, GL_FALSE,
                                  offsetof(RigidVertex, tang));
        glEnableVertexArrayAttrib(vertexArray, nextIndex++);

        glVertexArrayAttribBinding(vertexArray, nextIndex, bufferIndex);
        glVertexArrayAttribFormat(vertexArray, nextIndex, 2, GL_FLOAT, GL_TRUE,
                                  offsetof(RigidVertex, tex));
        glEnableVertexArrayAttrib(vertexArray, nextIndex++);
    };
};

template <>
struct VertexAttribs<DebugVertex> {
    static void setup(GLuint vertexArray, GLuint bufferIndex,
                      GLuint& nextIndex) {
        glVertexArrayAttribBinding(vertexArray, nextIndex, bufferIndex);
        glVertexArrayAttribFormat(vertexArray, nextIndex, 3, GL_FLOAT, GL_FALSE,
                                  offsetof(DebugVertex, pos));
        glEnableVertexArrayAttrib(vertexArray, nextIndex++);
    };
};

template <>
struct VertexAttribs<SkinVertex> {
    static void setup(GLuint vertexArray, GLuint bufferIndex,
                      GLuint& nextIndex) {
        glVertexArrayAttribBinding(vertexArray, nextIndex, bufferIndex);
        glVertexArrayAttribFormat(vertexArray, nextIndex, 3, GL_FLOAT, GL_FALSE,
                                  offsetof(SkinVertex, pos));
        glEnableVertexArrayAttrib(vertexArray, nextIndex++);

        glVertexArrayAttribBinding(vertexArray, nextIndex, bufferIndex);
        glVertexArrayAttribFormat(vertexArray, nextIndex, 3, GL_FLOAT, GL_FALSE,
                                  offsetof(SkinVertex, norm));
        glEnableVertexArrayAttrib(vertexArray, nextIndex++);

        glVertexArrayAttribBinding(vertexArray, nextIndex, bufferIndex);
        glVertexArrayAttribFormat(vertexArray, nextIndex, 4, GL_FLOAT, GL_FALSE,
                                  offsetof(SkinVertex, tang));
        glEnableVertexArrayAttrib(vertexArray, nextIndex++);

        glVertexArrayAttribBinding(vertexArray, nextIndex, bufferIndex);
        glVertexArrayAttribIFormat(vertexArray, nextIndex, 4, GL_UNSIGNED_INT,
                                   offsetof(SkinVertex, joints));
        glEnableVertexArrayAttrib(vertexArray, nextIndex++);

        glVertexArrayAttribBinding(vertexArray, nextIndex, bufferIndex);
        glVertexArrayAttribFormat(vertexArray, nextIndex, 4, GL_FLOAT, GL_TRUE,
                                  offsetof(SkinVertex, weights));
        glEnableVertexArrayAttrib(vertexArray, nextIndex++);

        glVertexArrayAttribBinding(vertexArray, nextIndex, bufferIndex);
        glVertexArrayAttribFormat(vertexArray, nextIndex, 2, GL_FLOAT, GL_TRUE,
                                  offsetof(SkinVertex, tex));
        glEnableVertexArrayAttrib(vertexArray, nextIndex++);
    };
};

template <>
struct InstanceAttribs<glm::vec3> {
    static void setup(GLuint vertexArray, GLuint bufferIndex,
                      GLuint& nextIndex) {
        glVertexArrayAttribBinding(vertexArray, nextIndex, bufferIndex);
        glVertexArrayAttribFormat(vertexArray, nextIndex, 3, GL_FLOAT, GL_FALSE,
                                  0);
        glEnableVertexArrayAttrib(vertexArray, nextIndex++);
    }
};

template <>
struct InstanceAttribs<glm::mat4> {
    static void setup(GLuint vertexArray, GLuint bufferIndex,
                      GLuint& nextIndex) {
        glVertexArrayAttribBinding(vertexArray, nextIndex, bufferIndex);
        glVertexArrayAttribFormat(vertexArray, nextIndex, 4, GL_FLOAT, GL_FALSE,
                                  0 * sizeof(glm::vec4));
        glEnableVertexArrayAttrib(vertexArray, nextIndex++);

        glVertexArrayAttribBinding(vertexArray, nextIndex, bufferIndex);
        glVertexArrayAttribFormat(vertexArray, nextIndex, 4, GL_FLOAT, GL_FALSE,
                                  1 * sizeof(glm::vec4));
        glEnableVertexArrayAttrib(vertexArray, nextIndex++);

        glVertexArrayAttribBinding(vertexArray, nextIndex, bufferIndex);
        glVertexArrayAttribFormat(vertexArray, nextIndex, 4, GL_FLOAT, GL_FALSE,
                                  2 * sizeof(glm::vec4));
        glEnableVertexArrayAttrib(vertexArray, nextIndex++);

        glVertexArrayAttribBinding(vertexArray, nextIndex, bufferIndex);
        glVertexArrayAttribFormat(vertexArray, nextIndex, 4, GL_FLOAT, GL_FALSE,
                                  3 * sizeof(glm::vec4));
        glEnableVertexArrayAttrib(vertexArray, nextIndex++);
    }
};

template <>
struct InstanceAttribs<DebugInstance> {
    static void setup(GLuint vertexArray, GLuint bufferIndex,
                      GLuint& nextIndex) {
        glVertexArrayAttribBinding(vertexArray, nextIndex, bufferIndex);
        glVertexArrayAttribFormat(vertexArray, nextIndex, 3, GL_FLOAT, GL_TRUE,
                                  offsetof(DebugInstance, color));
        glEnableVertexArrayAttrib(vertexArray, nextIndex++);

        glVertexArrayAttribBinding(vertexArray, nextIndex, bufferIndex);
        glVertexArrayAttribFormat(
            vertexArray, nextIndex, 4, GL_FLOAT, GL_FALSE,
            offsetof(DebugInstance, model) + sizeof(glm::vec4) * 0);
        glEnableVertexArrayAttrib(vertexArray, nextIndex++);

        glVertexArrayAttribBinding(vertexArray, nextIndex, bufferIndex);
        glVertexArrayAttribFormat(
            vertexArray, nextIndex, 4, GL_FLOAT, GL_FALSE,
            offsetof(DebugInstance, model) + sizeof(glm::vec4) * 1);
        glEnableVertexArrayAttrib(vertexArray, nextIndex++);

        glVertexArrayAttribBinding(vertexArray, nextIndex, bufferIndex);
        glVertexArrayAttribFormat(
            vertexArray, nextIndex, 4, GL_FLOAT, GL_FALSE,
            offsetof(DebugInstance, model) + sizeof(glm::vec4) * 2);
        glEnableVertexArrayAttrib(vertexArray, nextIndex++);

        glVertexArrayAttribBinding(vertexArray, nextIndex, bufferIndex);
        glVertexArrayAttribFormat(
            vertexArray, nextIndex, 4, GL_FLOAT, GL_FALSE,
            offsetof(DebugInstance, model) + sizeof(glm::vec4) * 3);
        glEnableVertexArrayAttrib(vertexArray, nextIndex++);
    }
};

}  // namespace gl