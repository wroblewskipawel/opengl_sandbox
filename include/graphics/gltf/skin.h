#pragma once

#define GLM_FORCE_RADIANS

#include <fx/gltf.h>

#include <filesystem>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <stack>
#include <unordered_map>
#include <vector>

#include "graphics/gltf/animation.h"

namespace gltf {

class Skin {
   public:
    Skin(const fx::gltf::Document& document, const fx::gltf::Skin& skin);

    Skin(const Skin&) = default;
    Skin(Skin&&) = default;

    Skin& operator=(const Skin&) = default;
    Skin& operator=(Skin&&) = default;

    std::vector<glm::mat4> jointMatrices(const glm::mat4& rootTransform,
                                         size_t animation, float time) const {
        std::vector<glm::mat4> result(m_joints.size());
        auto jointTransforms = m_animations[animation].getJointTransforms(time);

        for (size_t i{0}; i < m_joints.size(); i++) {
            auto& joint = m_joints[i];
            for (size_t j{0}; j < joint.numChildren; j++) {
                jointTransforms[joint.firstChild + j] =
                    jointTransforms[i] * jointTransforms[joint.firstChild + j];
            }
            result[m_meshNodeIndexMap.at(joint.nodeId)] =
                jointTransforms[i] * joint.inverseBind;
        }

        return result;
    };

   private:
    friend class Scene;
    friend class Document;

    static std::string getGltfUID(const std::filesystem::path& path,
                                  const fx::gltf::Document& document,
                                  size_t skinIndex);

    static std::vector<uint32_t> getGltfSortedNodes(
        const fx::gltf::Skin& skin) {
        auto nodes{skin.joints};
        std::sort(nodes.begin(), nodes.end());
        return nodes;
    }

    static std::unordered_map<int32_t, uint32_t> getInternalOrdering(
        const fx::gltf::Document& document, const fx::gltf::Skin& skin) {
        std::unordered_map<int32_t, uint32_t> hierarchy{};
        hierarchy.emplace(skin.skeleton, 0);
        uint32_t next{1};

        std::stack<uint32_t> to_visit;
        to_visit.push(skin.skeleton);
        while (!to_visit.empty()) {
            auto nodeID = to_visit.top();
            to_visit.pop();
            if (nodeID != -1) {
                for (auto child : document.nodes[nodeID].children) {
                    hierarchy.emplace(child, next++);
                    to_visit.push(child);
                }
            }
        }
        return hierarchy;
    }

    void registerAnimation(
        std::string name, const fx::gltf::Document& document,
        const fx::gltf::Animation& animation,
        const std::unordered_map<int32_t, uint32_t>& skinNodeOrdering) {
        m_animations.emplace_back(document, animation, skinNodeOrdering);
        m_animationMap.emplace(std::move(name), m_animations.size());
    };

    struct Joint {
        glm::mat4 inverseBind;
        glm::quat r;
        glm::vec3 t;
        glm::vec3 s;
        uint32_t firstChild;
        uint32_t numChildren;
        int32_t nodeId;
    };

    std::vector<Joint> m_joints;
    std::vector<Animation> m_animations;
    std::unordered_map<std::string, size_t> m_animationMap;
    std::unordered_map<int32_t, uint32_t> m_meshNodeIndexMap;
};

}  // namespace gltf
