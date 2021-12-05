#pragma once

#include <glad/glad.h>

#include <array>
#include <cstring>
#include <initializer_list>
#include <iostream>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std::string_literals;

namespace gl {

class Shader;

namespace shared {

const size_t MAX_GL_NAME_LENGTH = 512;

class Block {
   public:
    static void printUniformsLayouts();

    ~Block() {
        if (m_glBuffer != GL_NONE) {
            glDeleteBuffers(1, &m_glBuffer);
        }
    }

    Block(const Block&) = delete;

    Block(Block&& other)
        : m_name{other.m_name},
          m_layout{other.m_layout},
          m_data{std::move(other.m_data)},
          m_glBuffer{other.m_glBuffer} {
        other.m_glBuffer = GL_NONE;
    }

    Block& operator=(const Block&) = delete;
    Block& operator=(Block&&) = delete;

    template <typename T>
    struct Entry {
        Entry(const std::string& name, const T* data)
            : name{name}, data{data} {};
        const std::string& name;
        const T* data;
    };

    template <typename... Args>
    static Block build(const std::string& name, Entry<Args>... args) {
        auto layout = blockLayouts.find(name);
        if (layout != blockLayouts.end()) {
            const auto& blockInfo = layout->second;
            Block block{name, blockInfo};
            block.setUniforms(args...);
            block.flush();
            return block;
        }
        throw std::invalid_argument("Layout for uniform block ["s + name +
                                    "] not found"s);
    }

    template <typename T>
    void set(const std::string& name, const T* src, size_t index = 0) {
        auto item = m_layout.uniforms.find(name);
        if (item != m_layout.uniforms.end()) {
            const auto& info = item->second;
            size_t offset = info.offset;
            if (index != 0) {
                setArray_element(info, src, index);
            } else if (info.matrixStride.has_value()) {
                setMatrix(info, src);
            } else {
                setValue(offset, src);
            }
        } else {
            throw std::invalid_argument("Uniform [" + name +
                                        "] not found in uniform block");
        }
    }

    void flush() {
        glNamedBufferSubData(m_glBuffer, 0, m_layout.byteSize, m_data.data());
    }

   private:
    friend class gl::Shader;

    struct UniformInfo {
        GLenum type;
        GLint offset;
        std::optional<GLint> matrixStride;
        std::optional<std::pair<GLint, GLint>> arrayParams;
    };

    struct BlockInfo {
        GLsizei byteSize;
        std::unordered_map<std::string, UniformInfo> uniforms;
    };

    inline static std::unordered_map<std::string, BlockInfo> blockLayouts;

    static void queryUniformLayouts(GLuint glProgram);
    static std::unordered_map<std::string, UniformInfo> getUniformInfo(
        GLuint glProgram, const std::string& blockName,
        const std::vector<GLuint>& uniformIndices);

    Block(const std::string& name, const BlockInfo& layout)
        : m_name{name}, m_layout{layout} {
        m_data.resize(layout.byteSize);
        glCreateBuffers(1, &m_glBuffer);
        glNamedBufferStorage(m_glBuffer, layout.byteSize, NULL,
                             GL_DYNAMIC_STORAGE_BIT);
    };

    template <typename T>
    void setUniforms(Entry<T> info) {
        setUniform(info);
    }

    template <typename T, typename... Args>
    void setUniforms(Entry<T> info, Entry<Args>... args) {
        setUniform(info);
        setUniforms(args...);
    }

    template <typename T>
    void setUniform(Entry<T> param) {
        const auto& uniform = m_layout.uniforms.find(param.name);
        if (uniform != m_layout.uniforms.end()) {
            auto info = uniform->second;
            if (info.arrayParams.has_value()) {
                setArray(info, param.data);
            } else if (info.matrixStride.has_value()) {
                setMatrix(info, param.data);
            } else {
                setValue(info.offset, param.data);
            }
        } else {
            throw std::invalid_argument("Uniform [" + param.name +
                                        "] not found in uniform block");
        }
    }

    template <typename T>
    void setValue(size_t offset, const T* src) {
        std::memcpy(&m_data[offset], src, sizeof(T));
    }

    template <typename T>
    void setMatrix(const UniformInfo& info, const T* src) {
        auto matrixStride = info.matrixStride.value();
        auto columns = matrixColumns(info.type);
        auto elements = typeElements(info.type);

        if (columns == std::numeric_limits<size_t>::max() ||
            elements == std::numeric_limits<size_t>::max()) {
            throw std::invalid_argument(
                "Cannot infer parameters for type gl type: "s +
                std::to_string(info.type));
        }

        auto entrySize = sizeof(T) * elements;
        for (size_t c{0}; c < columns; c++) {
            std::memcpy(&m_data[info.offset + c * matrixStride],
                        &reinterpret_cast<const uint8_t*>(src)[c * entrySize],
                        entrySize);
        }
    }

    template <typename T>
    void setArray(const UniformInfo& info, const T* src) {
        auto [arraySize, arrayStride] = info.arrayParams.value();
        auto elements = typeElements(info.type);

        if (elements == std::numeric_limits<size_t>::max()) {
            throw std::invalid_argument(
                "Cannot infer type parameters for gl type: "s +
                std::to_string(info.type));
        }

        auto entrySize = sizeof(T) * elements;
        if (info.matrixStride.has_value()) {
            auto matStride = info.matrixStride.value();
            auto columns = matrixColumns(info.type);

            for (size_t n{0}; n < arraySize; n++) {
                auto entry_offset = info.offset + n * arrayStride;
                for (size_t c{0}; c < columns; c++) {
                    std::memcpy(&m_data[entry_offset + c * matStride],
                                &reinterpret_cast<const uint8_t*>(
                                    src)[(n * columns + c) * arraySize],
                                arraySize);
                }
            }
        } else {
            for (size_t n{0}; n < arraySize; n++) {
                std::memcpy(
                    &m_data[info.offset + n * arrayStride],
                    &reinterpret_cast<const uint8_t*>(src)[n * arraySize],
                    arraySize);
            }
        }
    }

    template <typename T>
    void setArray_element(const UniformInfo& info, const T* src, size_t index) {
        auto [array_size, arrayStride] = info.arrayParams.value();
        if (index < array_size) {
            auto num_elements = typeElements(info.type);

            if (num_elements == std::numeric_limits<size_t>::max()) {
                throw std::invalid_argument(
                    "Cannot infer type parameters for gl type: "s +
                    std::to_string(info.type));
            }

            auto arraySize = sizeof(T) * num_elements;
            if (info.matrixStride.has_value()) {
                auto matStride = info.matrixStride.value();
                auto columns = matrixColumns(info.type);

                auto entry_offset = info.offset + index * arrayStride;
                for (size_t c{0}; c < columns; c++) {
                    std::memcpy(
                        &m_data[entry_offset + c * matStride],
                        &reinterpret_cast<const uint8_t*>(src)[c * arraySize],
                        arraySize);
                }
            } else {
                std::memcpy(&m_data[info.offset + index * arrayStride], src,
                            arraySize);
            }
        } else {
            std::out_of_range("Uniform block array member out of range access");
        }
    }

    size_t constexpr typeElements(GLenum type) noexcept {
        switch (type) {
            case GL_FLOAT:
            case GL_DOUBLE:
            case GL_INT:
            case GL_UNSIGNED_INT:
            case GL_BOOL:
            case GL_SAMPLER_1D:
            case GL_SAMPLER_2D:
            case GL_SAMPLER_3D:
            case GL_SAMPLER_CUBE:
            case GL_SAMPLER_1D_SHADOW:
            case GL_SAMPLER_2D_SHADOW:
            case GL_SAMPLER_1D_ARRAY:
            case GL_SAMPLER_2D_ARRAY:
            case GL_SAMPLER_1D_ARRAY_SHADOW:
            case GL_SAMPLER_2D_ARRAY_SHADOW:
            case GL_SAMPLER_2D_MULTISAMPLE:
            case GL_SAMPLER_2D_MULTISAMPLE_ARRAY:
            case GL_SAMPLER_CUBE_SHADOW:
            case GL_SAMPLER_BUFFER:
            case GL_SAMPLER_2D_RECT:
            case GL_SAMPLER_2D_RECT_SHADOW:
                return 1;
            case GL_FLOAT_VEC2:
            case GL_DOUBLE_VEC2:
            case GL_INT_VEC2:
            case GL_UNSIGNED_INT_VEC2:
            case GL_BOOL_VEC2:
            case GL_FLOAT_MAT2:
            case GL_FLOAT_MAT3x2:
            case GL_FLOAT_MAT4x2:
            case GL_DOUBLE_MAT2:
            case GL_DOUBLE_MAT3x2:
            case GL_DOUBLE_MAT4x2:
                return 2;
            case GL_FLOAT_VEC3:
            case GL_DOUBLE_VEC3:
            case GL_INT_VEC3:
            case GL_UNSIGNED_INT_VEC3:
            case GL_BOOL_VEC3:
            case GL_FLOAT_MAT3:
            case GL_FLOAT_MAT2x3:
            case GL_FLOAT_MAT4x3:
            case GL_DOUBLE_MAT3:
            case GL_DOUBLE_MAT2x3:
            case GL_DOUBLE_MAT4x3:
                return 3;
            case GL_FLOAT_VEC4:
            case GL_DOUBLE_VEC4:
            case GL_INT_VEC4:
            case GL_UNSIGNED_INT_VEC4:
            case GL_BOOL_VEC4:
            case GL_FLOAT_MAT4:
            case GL_FLOAT_MAT3x4:
            case GL_FLOAT_MAT2x4:
            case GL_DOUBLE_MAT4:
            case GL_DOUBLE_MAT3x4:
            case GL_DOUBLE_MAT2x4:
                return 4;
            default:
                return std::numeric_limits<size_t>::max();
        }
    };

    size_t constexpr matrixColumns(GLenum type) noexcept {
        switch (type) {
            case GL_FLOAT_MAT2:
            case GL_FLOAT_MAT2x3:
            case GL_FLOAT_MAT2x4:
            case GL_DOUBLE_MAT2:
            case GL_DOUBLE_MAT2x3:
            case GL_DOUBLE_MAT2x4:
                return 2;
            case GL_FLOAT_MAT3:
            case GL_FLOAT_MAT3x2:
            case GL_FLOAT_MAT3x4:
            case GL_DOUBLE_MAT3:
            case GL_DOUBLE_MAT3x2:
            case GL_DOUBLE_MAT3x4:
                return 3;
            case GL_FLOAT_MAT4:
            case GL_FLOAT_MAT4x2:
            case GL_FLOAT_MAT4x3:
            case GL_DOUBLE_MAT4:
            case GL_DOUBLE_MAT4x2:
            case GL_DOUBLE_MAT4x3:
                return 4;
            default:
                return std::numeric_limits<size_t>::max();
        }
    };

    const std::string m_name;
    const BlockInfo& m_layout;

    GLuint m_glBuffer;
    std::vector<uint8_t> m_data;
};

}  // namespace shared

}  // namespace gl