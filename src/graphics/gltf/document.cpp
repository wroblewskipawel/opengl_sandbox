#include "rupture/graphics/gltf/document.h"

#include <glm/gtc/type_ptr.hpp>
#include <queue>
#include <unordered_map>
#include <vector>

#include "rupture/graphics/gltf/utils.h"

using namespace std::string_literals;

namespace gltf {

Document::Document(const std::filesystem::path& path) {
    auto document = [&]() -> fx::gltf::Document {
        auto ext = path.extension();
        if (ext == ".gltf"s) {
            return fx::gltf::LoadFromText(path);
        } else if (ext == ".glb"s) {
            return fx::gltf::LoadFromBinary(path);
        } else {
            throw std::invalid_argument("Invalid extension: " + path.string());
        }
    }();
    loadDocument(path, document);
}

std::unordered_map<uint32_t, uint32_t> Document::getNodeParentMap(
    const fx::gltf::Document& document) {
    std::unordered_map<uint32_t, uint32_t> parentMap{};
    for (size_t i{0}; i < document.nodes.size(); i++) {
        const auto& node = document.nodes[i];
        for (const auto& child : node.children) {
            parentMap.emplace(static_cast<uint32_t>(child),
                              static_cast<uint32_t>(i));
        }
    }
    return parentMap;
}

Document::NodeTransforms Document::getNodeTransforms(
    const fx::gltf::Document& document) {
    std::vector<glm::mat4> globalTransform{document.nodes.size(),
                                           glm::mat4{1.0f}};

    std::queue<int32_t> to_visit;
    to_visit.push(0);
    while (!to_visit.empty()) {
        auto currentNodeId = to_visit.front();
        to_visit.pop();
        if (currentNodeId >= 0) {
            auto gltfNode = document.nodes[currentNodeId];
            auto nodeMatrix = [gltfNode]() {
                auto nodeMatrix = glm::make_mat4(gltfNode.matrix.data());
                if (nodeMatrix == glm::mat4{1.0f}) {
                    glm::quat r;
                    glm::vec3 s;
                    glm::vec3 t;
                    std::memcpy(&r, &gltfNode.rotation, sizeof(glm::quat));
                    std::memcpy(&s, &gltfNode.scale, sizeof(glm::vec3));
                    std::memcpy(&t, &gltfNode.translation, sizeof(glm::vec3));
                    return glm::translate(glm::mat4{1.0f}, t) *
                           glm::mat4_cast(r) * glm::scale(glm::mat4{1.0f}, s);
                }
                return nodeMatrix;
            }();
            globalTransform[currentNodeId] *= nodeMatrix;
            for (const auto childNodeId : gltfNode.children) {
                globalTransform[childNodeId] = globalTransform[currentNodeId];
            }

            const auto& children = gltfNode.children;
            for (const auto& child : children) {
                to_visit.push(child);
            }
        }
    }

    std::vector<glm::mat4> inverseTransform{globalTransform.size()};
    std::transform(globalTransform.begin(), globalTransform.end(),
                   inverseTransform.begin(),
                   [](const glm::mat4& mat) { return glm::inverse(mat); });

    return {globalTransform, inverseTransform};
}

void Document::loadDocument(const std::filesystem::path& path,
                            const fx::gltf::Document& document) {
    m_name = path.stem().string();
    std::unordered_map<uint32_t, uint32_t> skinMap{};
    loadMaterials(path, document);
    loadAnimations(document, skinMap);
    loadMeshes(document, skinMap);
}

void Document::loadMaterials(const std::filesystem::path& path,
                             const fx::gltf::Document& document) {
    auto& textures = at<Texture>();
    textures.reserve(document.images.size());
    for (size_t i{0}; i < document.images.size(); i++) {
        const auto& image = document.images[i];
        textures.emplace_back(path, document, image);
    }

    auto& materials = at<pbrMaterial>();
    materials.reserve(document.materials.size());
    for (size_t i{0}; i < document.materials.size(); i++) {
        const auto& material = document.materials[i];
        materials.emplace_back(path, document, material);
    }
}

class AnimParserHelper {
   public:
    void registerSkin(size_t skinIndex, const fx::gltf::Skin& skin) {
        m_skinNodesOrdered.emplace(skinIndex, Skin::getGltfSortedNodes(skin));
    }

    void registerAnimation(size_t animationIndex,
                           const fx::gltf::Animation& annimation) {
        m_animationNodesOrdered.emplace(
            animationIndex, Animation::getGltfSortedNodes(annimation));
    }

    bool isAnimationApplicable(size_t skinIndex, size_t animationIndex) const {
        const auto& skinNodes = m_skinNodesOrdered.at(skinIndex);
        const auto& animNodes = m_animationNodesOrdered.at(animationIndex);
        return (std::includes(skinNodes.begin(), skinNodes.end(),
                              animNodes.begin(), animNodes.end()));
    }

   private:
    std::unordered_map<size_t, std::vector<uint32_t>> m_skinNodesOrdered{};
    std::unordered_map<size_t, std::vector<uint32_t>> m_animationNodesOrdered{};
};

void Document::loadAnimations(const fx::gltf::Document& document,
                              std::unordered_map<uint32_t, uint32_t>& skinMap) {
    auto parentMap = getNodeParentMap(document);
    auto nodeTransforms = getNodeTransforms(document);

    auto assocHelper = AnimParserHelper{};
    auto& skins = at<Skin>();
    for (size_t i{0}; i < document.skins.size(); i++) {
        const auto& skin = document.skins[i];
        assocHelper.registerSkin(i, skin);
        skins.emplace_back(document, skin, parentMap,
                           nodeTransforms.globalTransform,
                           nodeTransforms.inverseTransform);
    }

    for (size_t i{0}; i < document.animations.size(); i++) {
        const auto& animation = document.animations[i];
        assocHelper.registerAnimation(i, animation);
        for (size_t skinID{0}; skinID < skins.size(); skinID++) {
            if (assocHelper.isAnimationApplicable(skinID, i)) {
                auto animName = !animation.name.empty()
                                    ? animation.name
                                    : "Animation."s + std::to_string(i);
                skins[skinID].registerAnimation(std::move(animName), document,
                                                animation);
                continue;
            }
        }
    }

    for (size_t i{0}; i < document.nodes.size(); i++) {
        const auto& node = document.nodes[i];
        if (node.skin != -1 && node.mesh != -1) {
            skinMap.emplace(node.mesh, node.skin);
        }
    }
}

void Document::loadMeshes(
    const fx::gltf::Document& document,
    const std::unordered_map<uint32_t, uint32_t>& skinMap) {
    auto getMaterialIndex =
        [](const fx::gltf::Primitive& primitive) -> std::optional<uint32_t> {
        if (primitive.material != -1) {
            return primitive.material;
        } else {
            return std::nullopt;
        }
    };

    auto u32Checked = [](size_t size) {
        uint32_t u32{static_cast<uint32_t>(size)};
        if (u32 < size) {
            throw std::runtime_error("Unsigned overflow");
        }
        return u32;
    };

    auto& skinModels = getModels<SkinVertex>();
    auto& skinnedMeshes = at<Mesh<SkinVertex>>();
    for (auto& [meshID, skinID] : skinMap) {
        Model<SkinVertex> model{};
        model.skin = skinID;
        const auto& mesh = document.meshes[meshID];
        for (size_t j{0}; j < mesh.primitives.size(); j++) {
            const auto& primitive = mesh.primitives[j];
            skinnedMeshes.emplace_back(document, primitive);
            model.primitives.emplace_back(
                Primitive{u32Checked(skinnedMeshes.size() - 1),
                          getMaterialIndex(primitive)});
        }
        std::string modelName =
            !mesh.name.empty() ? mesh.name : "Mesh."s + std::to_string(meshID);
        skinModels.emplace(std::move(modelName), std::move(model));
    }

    auto& simpleModels = getModels<RigidVertex>();
    auto& simpleMeshes = at<Mesh<RigidVertex>>();
    for (size_t i{0}; i < document.meshes.size(); i++) {
        if (skinMap.count(i)) {
            continue;
        }
        Model<RigidVertex> model{};
        const auto& mesh = document.meshes[i];
        for (size_t j{0}; j < mesh.primitives.size(); j++) {
            const auto& primitive = mesh.primitives[j];
            simpleMeshes.emplace_back(document, primitive);
            model.primitives.emplace_back(
                Primitive{u32Checked(simpleMeshes.size() - 1),
                          getMaterialIndex(primitive)});
        }
        std::string modelName =
            !mesh.name.empty() ? mesh.name : "Mesh."s + std::to_string(i);
        simpleModels.emplace(std::move(modelName), std::move(model));
    }
}

}  // namespace gltf
