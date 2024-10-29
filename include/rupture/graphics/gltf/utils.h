#pragma once

#include <fx/gltf.h>

#include <functional>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <type_traits>
#include <vector>

using namespace std::string_literals;

namespace gltf {

inline bool hasAttribute(const fx::gltf::Primitive& primitive,
                         const std::string& attribName) {
    auto item = primitive.attributes.find(attribName);
    return item != primitive.attributes.end();
}

inline fx::gltf::Accessor::ComponentType attribCompType(
    const fx::gltf::Document& document, const fx::gltf::Primitive& primitive,
    const std::string& attributeName) {
    const auto& accessor =
        document.accessors[primitive.attributes.at(attributeName)];
    return accessor.componentType;
}

template <typename>
struct Type;

template <typename Target, typename Source = Target>
std::function<bool(Target&)> reader(const fx::gltf::Document& document,
                                    const fx::gltf::Accessor& accessor) {
    static_assert(std::is_pod<Target>::value && std::is_pod<Source>::value &&
                  std::is_assignable<Target, Source>::value);
    if (accessor.type != Type<Target>::type) {
        throw std::invalid_argument("Invalid accessor for type"s);
    }
    const auto& bufferView = document.bufferViews.at(accessor.bufferView);
    const auto& buffer = document.buffers.at(bufferView.buffer);
    const size_t count{accessor.count};

    size_t offset = bufferView.byteOffset + accessor.byteOffset;
    size_t stride =
        bufferView.byteStride ? bufferView.byteStride : sizeof(Source);
    size_t element = 0;

    return [=](Target& item) mutable {
        if (element < count) {
            if constexpr (std::is_same<Target, Source>::value) {
                std::memcpy(&item, &buffer.data[offset + stride * element++],
                            sizeof(Source));
            } else {
                Source tmp{};
                std::memcpy(&tmp, &buffer.data[offset + stride * element++],
                            sizeof(Source));
                item = tmp;
            }
            return true;
        } else {
            return false;
        }
    };
}

template <typename Target, typename Source = Target>
std::function<bool(Target&)> requiredAttrib(
    const fx::gltf::Document& document, const fx::gltf::Primitive& primitive,
    const std::string& attrib, size_t& size) {
    auto item = primitive.attributes.at(attrib);
    const auto& accessor = document.accessors[item];
    size = accessor.count;
    return reader<Target, Source>(document, accessor);
}

template <typename Target, typename Source = Target>
std::function<bool(Target&)> optionalAttrib(
    const fx::gltf::Document& document, const fx::gltf::Primitive& primitive,
    const std::string& attrib) {
    auto item = primitive.attributes.find(attrib);
    if (item != primitive.attributes.end()) {
        const auto& accessor = document.accessors[item->second];
        return reader<Target, Source>(document, accessor);
    }
    return [](Target&) { return true; };
}

template <typename Target, typename Source = Target>
std::vector<Target> readContiguous(const fx::gltf::Document& document,
                                   const fx::gltf::Accessor& accessor) {
    static_assert(std::is_pod<Target>::value && std::is_pod<Source>::value);
    // static_assert(std::is_pod<Target>::value && std::is_pod<Source>::value &&
    //               std::is_assignable<Target, Source>::value);
    if (accessor.type != Type<Target>::type) {
        throw std::invalid_argument("Invalid accessor for type"s);
    }

    const auto& bufferView = document.bufferViews.at(accessor.bufferView);
    const auto& buffer = document.buffers.at(bufferView.buffer);
    const size_t count{accessor.count};

    if (bufferView.byteStride != 0) {
        throw std::invalid_argument("Accessor refers to non contiguous data"s);
    }

    size_t offset = bufferView.byteOffset + accessor.byteOffset;

    std::vector<Target> target(count);
    if constexpr (std::is_same<Target, Source>::value) {
        std::memcpy(target.data(), &buffer.data[offset],
                    sizeof(Target) * count);
    } else {
        std::vector<Source> source(count);
        std::memcpy(source.data(), &buffer.data[offset],
                    sizeof(Source) * count);
        for (size_t i{0}; i < count; i++) {
            target[i] = source[i];
        }
    }
    return target;
}

template <>
struct Type<glm::vec2> {
    static constexpr fx::gltf::Accessor::Type type =
        fx::gltf::Accessor::Type::Vec2;
};

template <>
struct Type<glm::vec3> {
    static constexpr fx::gltf::Accessor::Type type =
        fx::gltf::Accessor::Type::Vec3;
};

template <>
struct Type<glm::vec4> {
    static constexpr fx::gltf::Accessor::Type type =
        fx::gltf::Accessor::Type::Vec4;
};

template <>
struct Type<glm::uvec4> {
    static constexpr fx::gltf::Accessor::Type type =
        fx::gltf::Accessor::Type::Vec4;
};

template <>
struct Type<glm::quat> {
    static constexpr fx::gltf::Accessor::Type type =
        fx::gltf::Accessor::Type::Vec4;
};

template <>
struct Type<glm::mat2> {
    static constexpr fx::gltf::Accessor::Type type =
        fx::gltf::Accessor::Type::Mat2;
};

template <>
struct Type<glm::mat3> {
    static constexpr fx::gltf::Accessor::Type type =
        fx::gltf::Accessor::Type::Mat3;
};

template <>
struct Type<glm::mat4> {
    static constexpr fx::gltf::Accessor::Type type =
        fx::gltf::Accessor::Type::Mat4;
};

template <typename T>
struct Type {
    static constexpr fx::gltf::Accessor::Type type =
        fx::gltf::Accessor::Type::Scalar;
};

}  // namespace gltf