#include "rupture/graphics/gltf/skin.h"

#include <cstring>
#include <iostream>
#include <queue>
#include <unordered_map>

#include "rupture/graphics/gltf/utils.h"

namespace gltf {

Skin::Skin(const fx::gltf::Document& document, const fx::gltf::Skin& skin,
           const std::unordered_map<uint32_t, uint32_t>& parentMap,
           const std::vector<glm::mat4>& globalTransforms,
           const std::vector<glm::mat4>& inverseTransforms) {
    m_rootTransform = skin.skeleton != -1
                          ? globalTransforms[parentMap.at(skin.skeleton)]
                          : glm::mat4{1.0f};

    std::unordered_map<uint32_t, glm::mat4> bindMap{};
    if (skin.inverseBindMatrices != -1) {
        const auto& bindAccesor = document.accessors[skin.inverseBindMatrices];
        auto inverseBind =
            gltf::readContiguous<glm::mat4>(document, bindAccesor);
        if (inverseBind.size() != skin.joints.size()) {
            throw std::runtime_error("Incomplete skin data");
        }
        for (size_t i{0}; i < skin.joints.size(); i++) {
            bindMap.emplace(skin.joints[i], inverseBind[i]);
        }
    } else {
        throw std::runtime_error("Incomplete skin data");
    }

    for (size_t i = 0; i < skin.joints.size(); i++) {
        m_skinIndexJointMap.emplace(skin.joints[i], i);
    }

    std::vector<glm::mat4> bindPose{skin.joints.size(), glm::mat4{1.0f}};

    std::queue<int32_t> to_visit;
    m_joints.reserve(skin.joints.size());
    to_visit.push(skin.skeleton);
    while (!to_visit.empty()) {
        auto currentNodeId = to_visit.front();
        to_visit.pop();
        if (currentNodeId != -1) {
            auto& joint = m_joints.emplace_back();
            joint.nodeId = static_cast<uint32_t>(currentNodeId);
            m_meshNodeIndexMap.emplace(joint.nodeId, m_joints.size() - 1);

            auto gltfNode = document.nodes[joint.nodeId];
            std::memcpy(&joint.r, &gltfNode.rotation, sizeof(glm::quat));
            std::memcpy(&joint.s, &gltfNode.scale, sizeof(glm::vec3));
            std::memcpy(&joint.t, &gltfNode.translation, sizeof(glm::vec3));

            joint.firstChild = m_joints.size() + to_visit.size();
            joint.numChildren = gltfNode.children.size();
            joint.inverseBind = bindMap.at(joint.nodeId);
            joint.name = gltfNode.name;

            const auto& children = gltfNode.children;
            for (const auto& child : children) {
                to_visit.push(child);
            }
        }
    }
};

std::string Skin::getGltfUID(const std::filesystem::path& path,
                             const fx::gltf::Document& document,
                             size_t skinIndex) {
    const auto& skin = document.skins.at(skinIndex);
    auto stem = path.stem().string();
    if (!skin.name.empty()) {
        return path.stem().string() + "." + skin.name;
    } else {
        return path.stem().string() + ".Skin" + std::to_string(skinIndex);
    }
}

}  // namespace gltf
