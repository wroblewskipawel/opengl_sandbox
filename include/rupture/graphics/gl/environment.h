#pragma once

#include "rupture/graphics/gl/texture.h"

namespace gl {

class Context;

namespace handle {

class Environment {
   public:
    static Environment null() {
        return Environment(std::numeric_limits<size_t>::max());
    }

    Environment(const Environment&) = default;
    Environment(Environment&&) = default;

    Environment& operator=(const Environment&) = default;
    Environment& operator=(Environment&&) = default;

    bool operator==(const Environment& rhs) const { return index == rhs.index; }
    bool operator!=(const Environment& rhs) const { return index != rhs.index; }

   private:
    friend gl::Context;
    Environment(size_t index) : index{index} {}

    size_t index;
};

}  // namespace handle

class Environment {
   public:
    Environment(const Environment&) = delete;
    Environment(Environment&&) = default;

    Environment& operator=(const Environment&) = delete;
    Environment& operator=(Environment&&) = default;

    Texture& environmentMap() { return m_environmentCubeMap; }
    Texture& irradianceMap() { return m_irradianceCubeMap; }
    Texture& specularMap() { return m_specularCubeMap; }

   private:
    friend class CubeRenderer;
    Environment(Texture&& environmentCubeMap, Texture&& irradianceCubeMap,
                Texture&& specularMap)
        : m_environmentCubeMap{std::move(environmentCubeMap)},
          m_irradianceCubeMap{std::move(irradianceCubeMap)},
          m_specularCubeMap{std::move(specularMap)} {};

    Texture m_environmentCubeMap;
    Texture m_irradianceCubeMap;
    Texture m_specularCubeMap;
};

}  // namespace gl