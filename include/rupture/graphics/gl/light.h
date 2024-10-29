#pragma once

#include <glad/glad.h>

#include <glm/glm.hpp>
#include <stdexcept>
#include <vector>

#include "rupture/graphics/gl/block/std140.h"
#include "rupture/graphics/gl/shader.h"

const size_t POINT_LIGHT_PACK_SIZE = 16;

using namespace std::string_literals;

namespace gl {

class Context;

namespace handle {

class Lighting {
   public:
    static Lighting null() {
        return Lighting(std::numeric_limits<size_t>::max());
    }

    Lighting(const Lighting&) = default;
    Lighting(Lighting&&) = default;

    Lighting& operator=(const Lighting&) = default;
    Lighting& operator=(Lighting&&) = default;

    bool operator==(const Lighting& rhs) const { return index == rhs.index; }
    bool operator!=(const Lighting& rhs) const { return index != rhs.index; }

   private:
    friend gl::Context;
    Lighting(size_t index) : index{index} {};

    size_t index;
};

}  // namespace handle

template <size_t size>
class PointLightPack : public UniformBlock {
   public:
    using LightData = std140::Block<glm::vec3, glm::vec3>;

    PointLightPack(const PointLightPack&) = delete;
    PointLightPack(PointLightPack&& other) : m_glUnifrom{other.m_glUnifrom} {
        other.m_glUnifrom = GL_NONE;
    };

    PointLightPack& operator=(const PointLightPack&) = delete;
    PointLightPack& operator=(PointLightPack&& other) {
        m_glUnifrom = other.m_glUnifrom;
        other.m_glUnifrom = GL_NONE;
        return *this;
    };

    ~PointLightPack() {
        if (m_glUnifrom != GL_NONE) {
            glDeleteBuffers(1, &m_glUnifrom);
        }
    }

    static const size_t maxLights{size};

   private:
    friend class Context;

    inline static const std::string name{"PointLightPack"s +
                                         std::to_string(size)};

    PointLightPack(const std::vector<glm::vec3>& positions,
                   const std::vector<glm::vec3>& intensities) {
        if (positions.size() > maxLights) {
            throw std::logic_error("Point light pack size exceeded"s);
        }
        if (positions.size() != intensities.size()) {
            throw std::logic_error("Invalid point light data specification"s);
        }
        std140::Block<GLuint, LightData[size]> uniformData;
        std140::get<0>(uniformData) = static_cast<GLuint>(positions.size());
        auto& lightArray = std140::get<1>(uniformData);
        for (size_t i{0}; i < positions.size(); i++) {
            std140::get<0>(lightArray[i]) = positions[i];
            std140::get<1>(lightArray[i]) = intensities[i];
        }

        glCreateBuffers(1, &m_glUnifrom);
        glNamedBufferStorage(m_glUnifrom, sizeof(uniformData), &uniformData,
                             GL_NONE);
    }

    const std::string& blockName() const override { return name; };
    GLuint glBuffer() const override { return m_glUnifrom; };

    GLuint m_glUnifrom;
};

using LightPack = PointLightPack<POINT_LIGHT_PACK_SIZE>;

}  // namespace gl