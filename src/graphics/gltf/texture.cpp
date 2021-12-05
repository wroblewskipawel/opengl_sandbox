#include "graphics/gltf/texture.h"

#define STB_IMAGE_IMPLEMENTATION

#include <stb_image.h>

#include <cstring>
#include <sstream>

#include "magic_enum.hpp"

using namespace magic_enum;
using namespace std::string_literals;

namespace gltf {

Texture::Texture(size_t width, size_t height, Format format)
    : m_format{format}, m_width{width}, m_height{height} {
    m_imageData.resize(m_width * m_height * enum_integer(m_format));
};

Texture::Texture(const std::filesystem::path& path, Format format)
    : m_format{format} {
    loadFile(path);
}
Texture::Texture(const uint8_t* bytes, size_t byteLength, Format format)
    : m_format{format} {
    loadBytes(bytes, byteLength);
};

Texture::Texture(const std::filesystem::path& path,
                 const fx::gltf::Document& document,
                 const fx::gltf::Image& image)
    : m_format(Format::RGBA) {
    if (image.IsEmbeddedResource() && !image.uri.empty()) {
        std::vector<uint8_t> imageData{};
        image.MaterializeData(imageData);
        loadBytes(imageData.data(), imageData.size());
    } else if (!image.uri.empty()) {
        loadFile(fx::gltf::detail::GetDocumentRootPath(path) / image.uri);
    } else {
        const auto& bufferView = document.bufferViews[image.bufferView];
        const auto& buffer = document.buffers[bufferView.buffer];
        loadBytes(&buffer.data[bufferView.byteOffset], bufferView.byteLength);
    }
}

void Texture::loadFile(const std::filesystem::path& path) {
    int width{}, height{}, comp{};
    int components = enum_integer(m_format);
    stbi_uc* image =
        stbi_load(path.c_str(), &width, &height, &comp, components);
    if (image) {
        size_t imageByteSize = width * height * components;
        m_imageData.resize(imageByteSize);
        std::memcpy(m_imageData.data(), image, imageByteSize);
        stbi_image_free(image);
        m_width = width;
        m_height = height;
    } else {
        throw std::runtime_error("Failed to load texture at: "s +
                                 path.string());
    }
};

void Texture::loadBytes(const uint8_t* bytes, size_t byteLength) {
    int width{}, height{}, comp{};
    int components = enum_integer(m_format);
    stbi_uc* image = stbi_load_from_memory(bytes, byteLength, &width, &height,
                                           &comp, components);
    if (image) {
        size_t imageByteSize = width * height * components;
        m_imageData.resize(imageByteSize);
        std::memcpy(m_imageData.data(), image, imageByteSize);
        stbi_image_free(image);
        m_width = width;
        m_height = height;
    } else {
        std::ostringstream ss{};
        ss << bytes;
        throw std::runtime_error("Failed to load texture from buffer at: "s +
                                 ss.str());
    }
}

std::string Texture::getGltfUID(const std::filesystem::path& path,
                                const fx::gltf::Document& document,
                                uint32_t index) {
    const auto& image = document.images.at(index);
    if (!image.IsEmbeddedResource() && !image.uri.empty()) {
        return image.uri;
    } else if (!image.name.empty()) {
        return path.stem().string() + "."s + image.name;
    } else {
        return path.stem().string() + ".Image."s + std::to_string(index);
    }
}

HDRTexture::HDRTexture(const std::filesystem::path& path) {
    int width{}, height{}, comp{};
    float* image = stbi_loadf(path.c_str(), &width, &height, &comp, 0);
    if (image) {
        size_t imageByteSize = width * height * comp * sizeof(float);
        m_imageData.resize(width * height * comp);
        std::memcpy(m_imageData.data(), image, imageByteSize);
        m_width = width;
        m_height = height;
        stbi_image_free(image);
    } else {
        throw std::runtime_error("Failed to load hdr texture at: "s +
                                 path.string());
    }
}

}  // namespace gltf
