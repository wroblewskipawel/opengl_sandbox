#include "graphics/gltf/skin.h"

#include <cstring>
#include <iostream>
#include <unordered_map>

#include "graphics/gltf/utils.h"

namespace gltf {

Skin::Skin(const fx::gltf::Document& document, const fx::gltf::Skin& skin) {
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

    for (size_t i{0}; i < skin.joints.size(); i++) {
        m_meshNodeIndexMap.emplace(skin.joints[i], static_cast<uint32_t>(i));
    }

    std::stack<std::pair<int32_t, Joint&>> to_visit;
    m_joints.reserve(skin.joints.size());
    to_visit.push({skin.skeleton, m_joints.emplace_back()});
    while (!to_visit.empty()) {
        auto [node, joint] = to_visit.top();
        to_visit.pop();
        if (node != -1) {
            uint32_t nodeID{static_cast<uint32_t>(node)};
            auto node = document.nodes[nodeID];
            joint.inverseBind = bindMap.at(nodeID);
            std::memcpy(&joint.r, &node.rotation, sizeof(glm::quat));
            std::memcpy(&joint.s, &node.scale, sizeof(glm::vec3));
            std::memcpy(&joint.t, &node.translation, sizeof(glm::vec3));
            joint.firstChild = m_joints.size();
            joint.numChildren = node.children.size();
            joint.nodeId = nodeID;
            for (auto child : node.children) {
                to_visit.push({child, m_joints.emplace_back()});
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
