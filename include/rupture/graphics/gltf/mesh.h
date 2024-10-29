#pragma once

#include <fx/gltf.h>

#include <cstring>
#include <filesystem>
#include <functional>
#include <glm/glm.hpp>
#include <magic_enum.hpp>
#include <optional>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "rupture/graphics/gltf/utils.h"
#include "rupture/graphics/vertex.h"

using namespace std::string_literals;
using namespace magic_enum;

namespace gltf {

enum class PrimitiveMode : uint8_t {
    Points = 0,
    Lines = 1,
    LineLoop = 2,
    LineStrip = 3,
    Triangles = 4,
    TriangleStrip = 5,
    TriangleFan = 6,
};

template <typename Vert>
class Mesh {
   public:
    Mesh(std::vector<Vert>&& vertices, std::vector<uint32_t>&& indices,
         PrimitiveMode mode = PrimitiveMode::Triangles)
        : m_vertices{std::move(vertices)},
          m_indices{std::move(indices)},
          m_mode{mode} {
        if (m_vertices.empty()) {
            throw std::logic_error("Invalid vertex data");
        }
    };

    Mesh(const fx::gltf::Document& document,
         const fx::gltf::Primitive& primitive)
        : m_mode{enum_integer(primitive.mode)} {
        if (primitive.indices != -1) {
            m_indices = loadIndexBuffer(
                document, document.accessors.at(primitive.indices));
        }
        m_vertices = loadVertices(document, primitive);
    };

    Mesh(const Mesh&) = default;
    Mesh(Mesh&&) = default;

    Mesh& operator=(const Mesh&) = default;
    Mesh& operator=(Mesh&&) = default;

    PrimitiveMode& mode() { return m_mode; }
    PrimitiveMode mode() const { return m_mode; }

    std::vector<Vert>& vertices() { return m_vertices; }
    const std::vector<Vert>& vertices() const { return m_vertices; }

    std::optional<std::vector<uint32_t>>& indices() { return m_indices; }
    const std::optional<std::vector<uint32_t>>& indices() const {
        return m_indices;
    }

   private:
    friend class Scene;

    static std::string getGltfUID(const std::filesystem::path& path,
                                  const fx::gltf::Document& document,
                                  size_t index) {
        const auto& mesh = document.meshes.at(index);
        if (!mesh.name.empty()) {
            return path.stem().string() + "."s + mesh.name;
        } else {
            return path.stem().string() + ".Mesh."s + std::to_string(index);
        }
    }

    template <typename T = Vert,
              typename std::enable_if<std::is_same<T, RigidVertex>::value,
                                      bool>::type = true>
    std::vector<Vert> loadVertices(const fx::gltf::Document& document,
                                   const fx::gltf::Primitive& primitive) {
        size_t size{};
        auto posReader = gltf::requiredAttrib<glm::vec3>(document, primitive,
                                                         "POSITION"s, size);
        auto normReader =
            gltf::optionalAttrib<glm::vec3>(document, primitive, "NORMAL"s);

        auto tangReader =
            gltf::optionalAttrib<glm::vec4>(document, primitive, "TANGENT"s);

        auto texReader = [&]() -> std::function<bool(glm::vec2&)> {
            size_t size{};
            if (gltf::hasAttribute(primitive, "TEXCOORD_0"s)) {
                switch (
                    gltf::attribCompType(document, primitive, "TEXCOORD_0"s)) {
                    case fx::gltf::Accessor::ComponentType::Float:
                        return gltf::requiredAttrib<glm::vec2>(
                            document, primitive, "TEXCOORD_0"s, size);
                    case fx::gltf::Accessor::ComponentType::UnsignedByte: {
                        auto accessor =
                            gltf::requiredAttrib<glm::vec2, glm::u8vec2>(
                                document, primitive, "TEXCOORD_0"s, size);
                        return [accessor](glm::vec2& item) mutable {
                            bool valid = accessor(item);
                            item /= std::numeric_limits<uint8_t>::max();
                            return valid;
                        };
                    }
                    case fx::gltf::Accessor::ComponentType::UnsignedShort: {
                        auto accessor =
                            gltf::requiredAttrib<glm::vec2, glm::u16vec2>(
                                document, primitive, "TEXCOORD_0"s, size);
                        return [accessor](glm::vec2& item) mutable {
                            bool valid = accessor(item);
                            item /= std::numeric_limits<uint16_t>::max();
                            return valid;
                        };
                    }
                    default:
                        throw std::runtime_error("Invalid gltf component type");
                }
            } else {
                return [](glm::vec2&) { return true; };
            }
        }();

        std::vector<Vert> vertices(size);
        for (auto& vert : vertices) {
            if (posReader(vert.pos) && normReader(vert.norm) &&
                tangReader(vert.tang) && texReader(vert.tex)) {
            } else {
                throw std::runtime_error("Mesh data incomplete"s);
            }
        }
        return vertices;
    }

    template <typename T = Vert,
              typename std::enable_if<std::is_same<T, SkinVertex>::value,
                                      bool>::type = true>
    std::vector<Vert> loadVertices(const fx::gltf::Document& document,
                                   const fx::gltf::Primitive& primitive) {
        size_t size{};
        auto posReader = gltf::requiredAttrib<glm::vec3>(document, primitive,
                                                         "POSITION"s, size);
        auto normReader =
            gltf::optionalAttrib<glm::vec3>(document, primitive, "NORMAL"s);

        auto tangReader =
            gltf::optionalAttrib<glm::vec4>(document, primitive, "TANGENT"s);

        auto texReader = [&]() -> std::function<bool(glm::vec2&)> {
            size_t size{};
            if (gltf::hasAttribute(primitive, "TEXCOORD_0"s)) {
                switch (
                    gltf::attribCompType(document, primitive, "TEXCOORD_0"s)) {
                    case fx::gltf::Accessor::ComponentType::Float:
                        return gltf::requiredAttrib<glm::vec2>(
                            document, primitive, "TEXCOORD_0"s, size);
                    case fx::gltf::Accessor::ComponentType::UnsignedByte: {
                        auto accessor =
                            gltf::requiredAttrib<glm::vec2, glm::u8vec2>(
                                document, primitive, "TEXCOORD_0"s, size);
                        return [accessor](glm::vec2& item) mutable {
                            bool valid = accessor(item);
                            item /= std::numeric_limits<uint8_t>::max();
                            return valid;
                        };
                    }
                    case fx::gltf::Accessor::ComponentType::UnsignedShort: {
                        auto accessor =
                            gltf::requiredAttrib<glm::vec2, glm::u16vec2>(
                                document, primitive, "TEXCOORD_0"s, size);
                        return [accessor](glm::vec2& item) mutable {
                            bool valid = accessor(item);
                            item /= std::numeric_limits<uint16_t>::max();
                            return valid;
                        };
                    }
                    default:
                        throw std::runtime_error("Invalid gltf component type");
                }
            } else {
                return [](glm::vec2&) { return true; };
            }
        }();

        auto jointsReader = [&]() {
            size_t size{};
            switch (gltf::attribCompType(document, primitive, "JOINTS_0"s)) {
                case fx::gltf::Accessor::ComponentType::UnsignedByte:
                    return gltf::requiredAttrib<glm::uvec4, glm::u8vec4>(
                        document, primitive, "JOINTS_0"s, size);
                case fx::gltf::Accessor::ComponentType::UnsignedShort:
                    return gltf::requiredAttrib<glm::uvec4, glm::u16vec4>(
                        document, primitive, "JOINTS_0"s, size);
                default:
                    throw std::runtime_error("Invalid gltf component type");
            };
        }();

        auto weightsReader = [&]() -> std::function<bool(glm::vec4&)> {
            size_t size{};
            switch (gltf::attribCompType(document, primitive, "WEIGHTS_0"s)) {
                case fx::gltf::Accessor::ComponentType::Float:
                    return gltf::requiredAttrib<glm::vec4>(document, primitive,
                                                           "WEIGHTS_0"s, size);
                case fx::gltf::Accessor::ComponentType::UnsignedByte: {
                    auto accessor =
                        gltf::requiredAttrib<glm::vec4, glm::u8vec4>(
                            document, primitive, "WEIGHTS_0"s, size);
                    return [accessor](glm::vec4& item) mutable {
                        bool valid = accessor(item);
                        item /= std::numeric_limits<uint8_t>::max();
                        return valid;
                    };
                }
                case fx::gltf::Accessor::ComponentType::UnsignedShort: {
                    auto accessor =
                        gltf::requiredAttrib<glm::vec4, glm::u16vec4>(
                            document, primitive, "WEIGHTS_0"s, size);
                    return [accessor](glm::vec4& item) mutable {
                        bool valid = accessor(item);
                        item /= std::numeric_limits<uint16_t>::max();
                        return valid;
                    };
                }
                default:
                    throw std::runtime_error("Invalid gltf component type");
            }
        }();

        std::vector<Vert> vertices(size);
        for (auto& vert : vertices) {
            if (posReader(vert.pos) && normReader(vert.norm) &&
                tangReader(vert.tang) && texReader(vert.tex) &&
                jointsReader(vert.joints) && weightsReader(vert.weights)) {
            } else {
                throw std::runtime_error("Mesh data incomplete"s);
            }
        }
        return vertices;
    }

    std::vector<uint32_t> loadIndexBuffer(const fx::gltf::Document& document,
                                          const fx::gltf::Accessor& accessor) {
        switch (accessor.componentType) {
            case fx::gltf::Accessor::ComponentType::UnsignedInt: {
                return gltf::readContiguous<uint32_t>(document, accessor);
            }
            case fx::gltf::Accessor::ComponentType::UnsignedShort: {
                return gltf::readContiguous<uint32_t, uint16_t>(document,
                                                                accessor);
            }
            case fx::gltf::Accessor::ComponentType::UnsignedByte: {
                return gltf::readContiguous<uint32_t, uint8_t>(document,
                                                               accessor);
            }
            default:
                throw std::runtime_error("Invalid gltf component type");
        }
    }

    std::vector<Vert> m_vertices;
    std::optional<std::vector<uint32_t>> m_indices;
    PrimitiveMode m_mode;
};

}  // namespace gltf
