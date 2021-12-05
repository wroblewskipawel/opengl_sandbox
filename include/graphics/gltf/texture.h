
#pragma once

#include <fx/gltf.h>

#include <filesystem>
#include <vector>

namespace gltf {

class Texture {
   public:
    enum class Format {
        Grey = 1,
        GreyAlpha = 2,
        RGB = 3,
        RGBA = 4,
    };
    Texture(size_t width, size_t height, Format format = Format::RGBA);
    Texture(const uint8_t* bytes, size_t byteLength,
            Format format = Format::RGBA);
    Texture(const std::filesystem::path& path, Format format = Format::RGBA);
    Texture(const std::filesystem::path& path,
            const fx::gltf::Document& document, const fx::gltf::Image& image);

    Texture(const Texture&) = default;
    Texture(Texture&&) = default;

    Texture& operator=(const Texture&) = default;
    Texture& operator=(Texture&&) = default;

    static Texture null() { return Texture(1, 1, Format::RGBA); };

    const std::vector<uint8_t>& data() const { return m_imageData; }

    size_t width() const { return m_width; }
    size_t height() const { return m_height; }
    Format format() const { return m_format; }

   private:
    friend class Scene;
    friend class pbrMaterial;

    static std::string getGltfUID(const std::filesystem::path& path,
                                  const fx::gltf::Document& document,
                                  uint32_t index);

    void loadFile(const std::filesystem::path& path);
    void loadBytes(const uint8_t* bytes, size_t byteLength);

    std::vector<uint8_t> m_imageData;
    Format m_format;
    size_t m_width;
    size_t m_height;
};

class HDRTexture {
   public:
    HDRTexture(const std::filesystem::path& path);

    HDRTexture(const HDRTexture&) = default;
    HDRTexture(HDRTexture&&) = default;

    HDRTexture& operator=(const HDRTexture&) = default;
    HDRTexture& operator=(HDRTexture&&) = default;

    const std::vector<float>& data() const { return m_imageData; };
    size_t width() const { return m_width; }
    size_t height() const { return m_height; }

   private:
    std::vector<float> m_imageData;
    size_t m_width;
    size_t m_height;
};

}  // namespace gltf
