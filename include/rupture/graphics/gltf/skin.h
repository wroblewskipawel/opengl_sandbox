#pragma once

#define GLM_FORCE_RADIANS

#include <fx/gltf.h>

#include <filesystem>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <iostream>
#include <stack>
#include <unordered_map>
#include <vector>

#include "rupture/graphics/gltf/animation.h"

namespace gltf {

class Skin {
   public:
    Skin(const fx::gltf::Document& document, const fx::gltf::Skin& skin,
         const std::unordered_map<uint32_t, uint32_t>& parentMap,
         const std::vector<glm::mat4>& globalTransforms,
         const std::vector<glm::mat4>& inverseTransforms);

    Skin(const Skin&) = default;
    Skin(Skin&&) = default;

    Skin& operator=(const Skin&) = default;
    Skin& operator=(Skin&&) = default;

    std::vector<glm::mat4> jointMatrices(const glm::mat4& rootTransform,
                                         size_t animation, float time) const {
        std::vector<glm::mat4> result(m_joints.size(), glm::mat4{1.0f});
        auto jointTransforms = m_animations[animation].getJointTransforms(time);

        for (size_t i{0}; i < m_joints.size(); i++) {
            auto& joint = m_joints[i];
            for (size_t j{0}; j < joint.numChildren; j++) {
                jointTransforms[joint.firstChild + j] =
                    jointTransforms[i] * jointTransforms[joint.firstChild + j];
            }
            result[m_skinIndexJointMap.at(joint.nodeId)] =
                m_rootTransform * jointTransforms[i] * joint.inverseBind;
        }

        return result;
    };

    std::vector<glm::mat4> bindPose() const {
        std::vector<glm::mat4> result(m_joints.size(), glm::mat4{1.0f});
        auto bindPose = getBindPose();

        for (size_t i{0}; i < m_joints.size(); i++) {
            auto& joint = m_joints[i];
            for (size_t j{0}; j < joint.numChildren; j++) {
                bindPose[joint.firstChild + j] =
                    bindPose[i] * bindPose[joint.firstChild + j];
            }
            result[m_skinIndexJointMap.at(joint.nodeId)] =
                m_rootTransform * bindPose[i] * joint.inverseBind;
        }

        return result;
    };

    double animationDuration(size_t animation) const {
        return m_animations[animation].duration();
    }

   private:
    friend class Scene;
    friend class Document;
    friend class AnimParserHelper;

    static std::string getGltfUID(const std::filesystem::path& path,
                                  const fx::gltf::Document& document,
                                  size_t skinIndex);

    static std::vector<uint32_t> getGltfSortedNodes(
        const fx::gltf::Skin& skin) {
        auto nodes{skin.joints};
        std::sort(nodes.begin(), nodes.end());
        return nodes;
    }

    void registerAnimation(std::string name, const fx::gltf::Document& document,
                           const fx::gltf::Animation& animation) {
        m_animations.emplace_back(document, animation, m_meshNodeIndexMap);
        m_animationMap.emplace(std::move(name), m_animations.size());
    };

    std::vector<glm::mat4> getBindPose() const {
        std::vector<glm::mat4> bindPose;
        bindPose.reserve(m_joints.size());
        for (const auto& joint : m_joints) {
            bindPose.push_back(glm::translate(glm::mat4{1.0f}, joint.t) *
                               glm::mat4_cast(joint.r) *
                               glm::scale(glm::mat4{1.0f}, joint.s));
        }
        return bindPose;
    }

    struct Joint {
        glm::mat4 inverseBind;
        glm::quat r;
        glm::vec3 t;
        glm::vec3 s;
        uint32_t firstChild;
        uint32_t numChildren;
        uint32_t nodeId;
        std::string name;
    };

    glm::mat4 m_rootTransform;
    std::vector<Joint> m_joints;
    std::vector<Animation> m_animations;
    std::unordered_map<std::string, size_t> m_animationMap;
    std::unordered_map<uint32_t, uint32_t> m_meshNodeIndexMap;
    std::unordered_map<uint32_t, uint32_t> m_skinIndexJointMap;
};

}  // namespace gltf
