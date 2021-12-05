#include "graphics/gltf/animation.h"

#include <cstring>
#include <iostream>
#include <string>

#include "graphics/gltf/utils.h"

namespace gltf {

Animation::Animation(
    const fx::gltf::Document& document, const fx::gltf::Animation& animation,
    const std::unordered_map<int32_t, uint32_t>& skinNodeOrdering) {
    m_targets.resize(skinNodeOrdering.size());
    for (size_t i{0}; i < animation.channels.size(); i++) {
        const auto& channel = animation.channels[i];
        const auto& sampler = animation.samplers[channel.sampler];
        auto property = [&]() {
            if (channel.target.path == "translation"s) {
                return Property::Translation;
            }
            if (channel.target.path == "rotation"s) {
                return Property::Rotation;
            }
            if (channel.target.path == "scale"s) {
                return Property::Scale;
            }
            throw std::runtime_error("Animation target unsupported");
        }();
        auto view = loadSamplerData(document, sampler, property);
        auto& target = m_targets[skinNodeOrdering.at(channel.target.node)];
        target.nodeId = channel.target.node;
        switch (property) {
            case Property::Translation:
                target.t = view;
                break;
            case Property::Rotation:
                target.r = view;
                break;
            case Property::Scale:
                target.s = view;
                break;
        }
    }
}

Animation::BufferView Animation::loadSamplerData(
    const fx::gltf::Document& document,
    const fx::gltf::Animation::Sampler& sampler, Property property) {
    const auto& input = document.accessors[sampler.input];
    const auto& output = document.accessors[sampler.output];

    BufferView view{};
    view.mode = [&]() {
        switch (sampler.interpolation) {
            case fx::gltf::Animation::Sampler::Type::Linear:
                return Interpolation::Linear;
            case fx::gltf::Animation::Sampler::Type::Step:
                return Interpolation::Step;
            case fx::gltf::Animation::Sampler::Type::CubicSpline:
                return Interpolation::CubicSpline;
            default:
                throw std::runtime_error("Invalid interpolation value"s);
        };
    }();
    auto inputData = gltf::readContiguous<float>(document, input);
    view.samples = inputData.size();
    switch (property) {
        case Property::Translation: {
            view.inputOffset = m_translationBuffer.timeBuffer.size();
            std::copy(inputData.begin(), inputData.end(),
                      std::back_inserter(m_translationBuffer.timeBuffer));
            auto outputData = gltf::readContiguous<glm::vec3>(document, output);
            view.outputOffset = m_translationBuffer.targetBuffer.size();
            std::copy(outputData.begin(), outputData.end(),
                      std::back_inserter(m_translationBuffer.targetBuffer));
            break;
        }
        case Property::Rotation: {
            view.inputOffset = m_rotationBuffer.timeBuffer.size();
            std::copy(inputData.begin(), inputData.end(),
                      std::back_inserter(m_rotationBuffer.timeBuffer));
            auto outputData = gltf::readContiguous<glm::quat>(document, output);
            view.outputOffset = m_rotationBuffer.targetBuffer.size();
            std::copy(outputData.begin(), outputData.end(),
                      std::back_inserter(m_rotationBuffer.targetBuffer));
            break;
        }
        case Property::Scale: {
            view.inputOffset = m_scaleBuffer.timeBuffer.size();
            std::copy(inputData.begin(), inputData.end(),
                      std::back_inserter(m_scaleBuffer.timeBuffer));
            auto outputData = gltf::readContiguous<glm::vec3>(document, output);
            view.outputOffset = m_scaleBuffer.targetBuffer.size();
            std::copy(outputData.begin(), outputData.end(),
                      std::back_inserter(m_scaleBuffer.targetBuffer));
            break;
        }
    }
    return view;
}

}  // namespace gltf
