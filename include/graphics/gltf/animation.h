#pragma once

#define GLM_FORCE_RADIANS

#include <fx/gltf.h>

#include <algorithm>
#include <array>
#include <filesystem>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <optional>
#include <set>
#include <unordered_map>
#include <vector>

using namespace std::string_literals;

namespace gltf {

class Animation {
   public:
    Animation(const fx::gltf::Document& document,
              const fx::gltf::Animation& animation,
              const std::unordered_map<int32_t, uint32_t>& skinNodeOrdering);

    Animation(const Animation&) = default;
    Animation(Animation&&) = default;

    Animation& operator=(const Animation&) = default;
    Animation& operator=(Animation&&) = default;

   private:
    friend class Scene;
    friend class Skin;
    friend class Document;

    static std::string getGltfUID(const std::filesystem::path& path,
                                  const fx::gltf::Document& document,
                                  size_t animationIndex) {
        const auto& animation = document.animations[animationIndex];
        if (!animation.name.empty()) {
            return path.stem().string() + "."s + animation.name;
        } else {
            return path.stem().string() + ".Animation"s +
                   std::to_string(animationIndex);
        }
    }

    static std::vector<uint32_t> getGltfSortedNodes(
        const fx::gltf::Animation& animation) {
        std::set<uint32_t> nodes{};
        for (size_t i{0}; i < animation.channels.size(); i++) {
            nodes.insert(animation.channels[i].target.node);
        }
        return {nodes.begin(), nodes.end()};
    }

    enum class Property {
        Rotation,
        Translation,
        Scale,
    };

    enum class Interpolation {
        Linear,
        Step,
        CubicSpline,
    };

    struct BufferView {
        uint32_t inputOffset;
        uint32_t outputOffset;
        uint32_t samples;
        Interpolation mode;
    };

    BufferView loadSamplerData(const fx::gltf::Document& document,
                               const fx::gltf::Animation::Sampler& sampler,
                               Property property);

    glm::mat4 targetTransform(uint32_t nodeIndex, float time) const {
        auto& node = m_targets[nodeIndex];
        auto t = node.t.has_value()
                     ? m_translationBuffer.sample(node.t.value(), time)
                     : glm::vec3{0.0f};
        auto r = node.r.has_value()
                     ? m_rotationBuffer.sample(node.r.value(), time)
                     : glm::quat{1.0f, 0.0f, 0.0f, 0.0f};
        auto s = node.s.has_value() ? m_scaleBuffer.sample(node.s.value(), time)
                                    : glm::vec3{1.0f};
        auto angle = glm::angle(r);
        auto axis = glm::axis(r);
        return glm::scale(
            glm::rotate(glm::translate(glm::mat4{1.0f}, t), angle, axis), s);
    }

    std::vector<glm::mat4> getJointTransforms(float time) const {
        std::vector<glm::mat4> transforms(m_targets.size());
        for (size_t i{0}; i < m_targets.size(); i++) {
            transforms[i] = targetTransform(i, time);
        }
        return transforms;
    }

    struct Target {
        std::optional<BufferView> t;
        std::optional<BufferView> r;
        std::optional<BufferView> s;
        int32_t nodeId;
    };

    template <typename Comp>
    struct AnimationBuffer {
        std::vector<float> timeBuffer;
        std::vector<Comp> targetBuffer;

        Comp sample(const BufferView& view, float time) const {
            auto offset = std::distance(
                timeBuffer.begin() + view.inputOffset,
                std::lower_bound(
                    timeBuffer.begin() + view.inputOffset,
                    timeBuffer.begin() + view.inputOffset + view.samples,
                    time));
            if (offset != view.samples && offset > 0) {
                float t = (time - timeBuffer[view.inputOffset + offset - 1]) /
                          (timeBuffer[view.inputOffset + offset] -
                           timeBuffer[view.inputOffset + offset - 1]);
                switch (view.mode) {
                    case Interpolation::Step:
                        return targetBuffer[view.outputOffset + offset - 1];
                    case Interpolation::Linear: {
                        if constexpr (std::is_same<glm::quat, Comp>::value) {
                            return glm::slerp(
                                targetBuffer[view.outputOffset + offset - 1],
                                targetBuffer[view.outputOffset + offset], t);
                        } else {
                            return glm::mix(
                                targetBuffer[view.outputOffset + offset - 1],
                                targetBuffer[view.outputOffset + offset], t);
                        }
                    }
                    default:
                        throw std::runtime_error(

                            "Interpolation method not implemented");
                }
            } else if (offset == 0) {
                return targetBuffer[view.outputOffset];
            } else {
                return targetBuffer[view.outputOffset + offset - 1];
            }
        }
    };

    AnimationBuffer<glm::vec3> m_translationBuffer;
    AnimationBuffer<glm::vec3> m_scaleBuffer;
    AnimationBuffer<glm::quat> m_rotationBuffer;

    std::vector<Target> m_targets;
};

}  // namespace gltf
