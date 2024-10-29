#pragma once

#include <glm/glm.hpp>
#include <vector>

struct RigidVertex {
    glm::vec3 pos;
    glm::vec3 norm;
    glm::vec4 tang;
    glm::vec2 tex;
};

struct SkinVertex {
    glm::vec3 pos;
    glm::vec3 norm;
    glm::vec4 tang;
    glm::uvec4 joints;
    glm::vec4 weights;
    glm::vec2 tex;
};

struct DebugVertex {
    glm::vec3 pos;
};

struct DebugInstance {
    glm::vec3 color;
    glm::mat4 model;
};
