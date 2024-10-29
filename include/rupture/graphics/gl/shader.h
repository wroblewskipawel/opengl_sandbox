#pragma once

#include <glad/glad.h>

#include <filesystem>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <unordered_map>

using namespace std::string_literals;

namespace gl {

class Context;

namespace handle {

class Shader {
   public:
    static Shader null() { return Shader(std::numeric_limits<size_t>::max()); }

    Shader(const Shader&) = default;
    Shader(Shader&&) = default;

    Shader& operator=(const Shader&) = default;
    Shader& operator=(Shader&&) = default;

    bool operator==(const Shader& rhs) const { return index == rhs.index; }
    bool operator!=(const Shader& rhs) const { return index != rhs.index; }

   private:
    friend gl::Context;
    Shader(size_t index) : index{index} {};

    size_t index;
};

}  // namespace handle

class UniformBlock {
   public:
    class Info {
        friend class Shader;
        friend class UniformBlock;

        Info(const std::string& name, GLuint glBuffer)
            : name{name}, glBuffer{glBuffer} {};
        const std::string& name;
        GLuint glBuffer;
    };
    Info blockInfo() const { return Info{blockName(), glBuffer()}; };

   protected:
    virtual const std::string& blockName() const = 0;
    virtual GLuint glBuffer() const = 0;
};

class Shader {
   public:
    inline static const std::string CAMERA_MATRIX_UNIFORM{"proj_view"s};
    inline static const std::string CAMERA_POS_UNIFORM{"camera_pos"s};
    inline static const std::string IRRADIANCE_CUBE_MAP{"irradiance_map"s};
    inline static const std::string SPECULAR_CUBE_MAP{"specular_map"s};
    inline static const std::string BRDF_MAP{"brdf_map"s};

    enum class Stage : GLenum {
        VERTEX = GL_VERTEX_SHADER,
        FRAGMENT = GL_FRAGMENT_SHADER,
        TESS_CONT = GL_TESS_CONTROL_SHADER,
        TESS_EVAL = GL_TESS_EVALUATION_SHADER,
        GEOMETRY = GL_GEOMETRY_SHADER,
        COMPUTE = GL_COMPUTE_SHADER,
    };

    Shader(const std::filesystem::path& dir);

    Shader(const Shader&) = delete;
    Shader(Shader&& other)
        : m_glProgram{other.m_glProgram},
          m_uniformLocations{std::move(other.m_uniformLocations)},
          m_blockBindings{std::move(other.m_blockBindings)} {
        other.m_glProgram = GL_NONE;
    }

    Shader& operator=(const Shader&) = delete;
    Shader& operator=(Shader&& other) {
        m_glProgram = other.m_glProgram;
        m_uniformLocations = std::move(other.m_uniformLocations);
        m_blockBindings = std::move(other.m_blockBindings);
        other.m_glProgram = GL_NONE;
        return *this;
    };

    ~Shader() {
        if (m_glProgram != GL_NONE) {
            if (m_glProgram == currnetProgram) {
                currnetProgram = GL_NONE;
            }
            glDeleteProgram(m_glProgram);
        }
    }

    void bindUniformBlock(const UniformBlock::Info& blockInfo) {
        if (m_glProgram == currnetProgram) {
            glBindBufferBase(GL_UNIFORM_BUFFER,
                             m_blockBindings.at(blockInfo.name),
                             blockInfo.glBuffer);
        } else {
            throw std::logic_error("OpenGL program not currently in use");
        }
    }

    void setUniformF(const std::string& name, float value) {
        if (m_glProgram == currnetProgram) {
            glUniform1f(m_uniformLocations.at(name), value);
        } else {
            throw std::logic_error("OpenGL Program not currently in use");
        }
    }

    void setUniformI(const std::string& name, GLint value) {
        if (m_glProgram == currnetProgram) {
            glUniform1i(m_uniformLocations.at(name), value);
        } else {
            throw std::logic_error("OpenGL Program not currently in use");
        }
    }

    void setUniformUI(const std::string& name, GLuint value) {
        if (m_glProgram == currnetProgram) {
            glUniform1ui(m_uniformLocations.at(name), value);
        } else {
            throw std::logic_error("OpenGL program not currently in use");
        }
    }

    void setUniformMatrix4(const std::string& name, const glm::mat4& value) {
        if (m_glProgram == currnetProgram) {
            glUniformMatrix4fv(m_uniformLocations.at(name), 1, GL_FALSE,
                               glm::value_ptr(value));
        } else {
            throw std::logic_error("OpenGL program not currently in use");
        }
    }

    void setUniformVec3(const std::string& name, const glm::vec3& value) {
        if (m_glProgram == currnetProgram) {
            glUniform3fv(m_uniformLocations.at(name), 1, glm::value_ptr(value));
        } else {
            throw std::logic_error("OpenGL program not currently in use");
        }
    }

    void use() {
        if (m_glProgram != currnetProgram) {
            glUseProgram(m_glProgram);
            currnetProgram = m_glProgram;
        }
    }

   private:
    using Source =
        std::unordered_map<Shader::Stage, const std::filesystem::path>;

    void load(const Source& source);
    std::string loadSource(const std::filesystem::path& filepath);
    GLuint loadShader(Stage stage, const std::filesystem::path& filepath);

    void interfaceQuery();

    std::unordered_map<std::string, GLuint> m_blockBindings;
    std::unordered_map<std::string, GLint> m_uniformLocations;

    inline static GLuint currnetProgram{GL_NONE};
    GLuint m_glProgram;
};

}  // namespace gl