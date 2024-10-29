#include "rupture/graphics/gltf/document.h"

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

void Document::loadAnimations(const fx::gltf::Document& document,
                              std::unordered_map<uint32_t, uint32_t>& skinMap) {
    std::unordered_map<size_t, std::vector<uint32_t>> skinNodes{};
    auto& skins = at<Skin>();
    for (size_t i{0}; i < document.skins.size(); i++) {
        const auto& skin = document.skins[i];
        skinNodes.emplace(i, Skin::getGltfSortedNodes(skin));
        skins.emplace_back(document, skin);
    }

    for (size_t i{0}; i < document.animations.size(); i++) {
        const auto& animation = document.animations[i];
        auto animNodes = Animation::getGltfSortedNodes(animation);
        for (auto& [skinID, nodes] : skinNodes) {
            if (std::includes(nodes.begin(), nodes.end(), animNodes.begin(),
                              animNodes.end())) {
                auto orderedNodes =
                    Skin::getInternalOrdering(document, document.skins[skinID]);
                auto animName = !animation.name.empty()
                                    ? animation.name
                                    : "Animation."s + std::to_string(i);
                skins[skinID].registerAnimation(std::move(animName), document,
                                                animation, orderedNodes);
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
