#include "graphics/gl/shader.h"

#include <array>
#include <exception>
#include <fstream>
#include <iostream>
#include <sstream>

#include "graphics/gl/block/shared.h"

namespace gl {

const GLsizei glBufferLen = 512;
GLchar glBuffer[glBufferLen];

Shader::Shader(const std::filesystem::path& dir) {
    Source shaderSource{};
    for (const auto& entry : std::filesystem::directory_iterator{dir}) {
        if (entry.is_regular_file()) {
            const auto filepath = entry.path();
            const auto ext = filepath.extension();
            Stage stage = [&]() {
                if (ext == ".vert"s) {
                    return Stage::VERTEX;
                } else if (ext == ".frag"s) {
                    return Stage::FRAGMENT;
                } else if (ext == ".tesc"s) {
                    return Stage::TESS_CONT;
                } else if (ext == ".tese"s) {
                    return Stage::TESS_EVAL;
                } else if (ext == ".geom"s) {
                    return Stage::GEOMETRY;
                } else if (ext == ".comp") {
                    return Stage::COMPUTE;
                }
                throw std::runtime_error("Invalid shader file extension");
            }();
            auto [_, inserted] = shaderSource.try_emplace(stage, filepath);
            if (!inserted) {
                throw std::runtime_error(
                    "Shader directory contains multiple definitions for same "
                    "pipeline stage");
            }
        }
    }
    load(shaderSource);
    interfaceQuery();
}

void Shader::load(const Source& source) {
    GLint status{};
    std::array<GLuint, 6> shaders{};
    m_glProgram = glCreateProgram();
    size_t n{0};
    try {
        for (auto& [stage, filepath] : source) {
            shaders[n] = loadShader(stage, filepath);
            if (shaders[n] != 0) {
                glAttachShader(m_glProgram, shaders[n++]);
            }
        }
        glLinkProgram(m_glProgram);
        glGetProgramiv(m_glProgram, GL_LINK_STATUS, &status);
        if (status != GL_TRUE) {
            glGetProgramInfoLog(m_glProgram, glBufferLen, NULL, glBuffer);
            std::string errorMessage{"Failed to link program\n"s + glBuffer};
            throw std::runtime_error(errorMessage);
        }
    } catch (const std::exception& e) {
        glDeleteProgram(m_glProgram);
        for (size_t i{0}; i < n; i++) glDeleteShader(shaders[i]);
        throw;
    }
    for (size_t i{0}; i < n; i++) glDeleteShader(shaders[i]);
    shared::Block::queryUniformLayouts(m_glProgram);
}

std::string Shader::loadSource(const std::filesystem::path& filepath) {
    std::ifstream fs{};
    std::ostringstream os{};
    fs.exceptions(std::ios::failbit | std::ios::badbit);
    os.exceptions(std::ios::failbit | std::ios::badbit);
    try {
        fs.open(filepath);
        os << fs.rdbuf();
        fs.close();
    } catch (std::exception& e) {
        throw;
    }
    return os.str();
}

GLuint Shader::loadShader(Stage stage, const std::filesystem::path& filepath) {
    GLint status{};
    GLuint shader = glCreateShader(static_cast<GLenum>(stage));
    try {
        std::string source = loadSource(filepath);
        auto* c_str = source.c_str();
        auto lenght = static_cast<GLint>(source.length());
        if (shader != 0) {
            glShaderSource(shader, 1, &c_str, &lenght);
            glCompileShader(shader);
            glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
            if (status != GL_TRUE) {
                glGetShaderInfoLog(shader, glBufferLen, NULL, glBuffer);
                std::string errorMessage{"Failed to compile shader [" +
                                         filepath.string() + "]\n" + glBuffer};
                throw std::runtime_error(errorMessage);
            }
            return shader;
        } else {
            throw std::runtime_error("Failed to initialize shader");
        }
    } catch (std::exception& e) {
        glDeleteShader(shader);
        throw;
    }
    return 0;
}

void Shader::interfaceQuery() {
    GLint uniformBlockCount{};
    glGetProgramInterfaceiv(m_glProgram, GL_UNIFORM_BLOCK, GL_ACTIVE_RESOURCES,
                            &uniformBlockCount);
    for (size_t index{0}; index < uniformBlockCount; index++) {
        glGetProgramResourceName(m_glProgram, GL_UNIFORM_BLOCK, index,
                                 glBufferLen, NULL, glBuffer);
        glUniformBlockBinding(m_glProgram, index, index);
        m_blockBindings.emplace(glBuffer, index);
    }

    for (const auto& [name, binding] : m_blockBindings) {
        std::cout << "Uniform: " << name << " ; Binding: " << binding
                  << std::endl;
    }

    GLint uniformCount{};
    glGetProgramInterfaceiv(m_glProgram, GL_UNIFORM, GL_ACTIVE_RESOURCES,
                            &uniformCount);
    std::vector<GLuint> uniformIndices(uniformCount);
    for (uint32_t i{0}; i < uniformCount; i++) {
        uniformIndices[i] = i;
    }
    std::vector<GLint> uniform_blocks(uniformCount);
    glGetActiveUniformsiv(m_glProgram, uniformCount, uniformIndices.data(),
                          GL_UNIFORM_BLOCK_INDEX, uniform_blocks.data());
    std::vector<GLint> array_strides(uniformCount);
    glGetActiveUniformsiv(m_glProgram, uniformCount, uniformIndices.data(),
                          GL_UNIFORM_ARRAY_STRIDE, array_strides.data());

    for (size_t index{0}; index < uniformCount; index++) {
        if (uniform_blocks[index] == -1 && array_strides[index] < 1) {
            glGetProgramResourceName(m_glProgram, GL_UNIFORM, index,
                                     glBufferLen, NULL, glBuffer);
            std::string uniformName{glBuffer};
            m_uniformLocations.emplace(
                glBuffer,
                glGetUniformLocation(m_glProgram, uniformName.c_str()));
        }
    }

    for (const auto& [name, location] : m_uniformLocations) {
        std::cout << "Uniform: " << name << " ; Location: " << location
                  << std::endl;
    }
}

}  // namespace gl