#pragma once

#include <glad/glad.h>

#include <filesystem>
#include <unordered_map>
#include <vector>

#include "graphics/camera.h"
#include "graphics/gl/environment.h"
#include "graphics/gl/framebuffer.h"
#include "graphics/gl/light.h"
#include "graphics/gl/material_pack.h"
#include "graphics/gl/model.h"
#include "graphics/gl/renderer.h"
#include "graphics/gl/shader.h"
#include "graphics/gl/texture.h"
#include "graphics/gl/uniform.h"
#include "graphics/gl/vertex_buffer.h"
#include "graphics/gltf/document.h"
#include "graphics/gltf/material.h"
#include "graphics/gltf/mesh.h"
#include "graphics/gltf/texture.h"
#include "graphics/window.h"
#include "typemap.h"

class Application;

namespace gl {

class Context;

class Context {
   public:
    Context(const Context&) = delete;
    Context(Context&&) = delete;

    Context& operator=(const Context&) = delete;
    Context& operator=(Context&&) = delete;

    ~Context() = default;

    void loadShaders(const std::vector<std::filesystem::path>& shaderDirs);
    void loadDocument(const gltf::Document& document);

    handle::Shader getShader(const std::string& name) {
        return m_shaderMap.at(name);
    }

    template <typename Vert>
    handle::Model<Vert> getModel(const std::string& name) {
        return m_modelMaps
            .at<std::unordered_map<std::string, handle::Model<Vert>>>()
            .at(name);
    }

    template <typename Vert>
    void loadModels(const std::vector<std::string>& modelNames,
                    const std::vector<gltf::Mesh<Vert>>& models) {
        if (modelNames.size() != models.size()) {
            throw std::logic_error("Invalid model load arguments");
        }
        auto u32Checked = [](size_t size) {
            uint32_t u32{static_cast<uint32_t>(size)};
            if (u32 < size) {
                throw std::runtime_error("Unsigned overflow");
            }
            return u32;
        };

        auto& handles = getModelHandles<Vert>();
        auto& glModels = m_models.at<std::vector<Model<Vert>>>();

        auto& vertexBuffers = at<VertexBuffer<Vert>>();
        auto& buffer = vertexBuffers.emplace_back(VertexBuffer{models});
        auto bufferIndex = u32Checked(vertexBuffers.size() - 1);
        for (size_t i{0}; i < models.size(); i++) {
            Model<Vert> glModel{};
            auto offsets = buffer.getOffset(i);
            glModel.drawInfos.emplace_back(DrawInfo<Vert>{
                bufferIndex, 0, 0, offsets.baseVertex, offsets.indexPointer,
                offsets.numIndices, offsets.mode});
            glModels.push_back(glModel);
            handles.emplace(modelNames[i],
                            handle::Model<Vert>{glModels.size() - 1});
        }
    };

    void setShaderState(handle::Shader handle);
    void setEnvironment(handle::Environment handle);
    void setLighting(handle::Lighting handle);

    template <typename VertexType, typename InstanceType>
    void draw(handle::Model<VertexType> modelHandle,
              const InstanceType& instance) {
        if (m_frameState.shader == handle::Shader::null()) {
            throw std::logic_error("Shader not set for current frame");
        }
        auto& renderer = m_renderes.at<Renderer<VertexType, InstanceType>>();

        auto& model = getDrawInfo(modelHandle);
        auto& baseDrawInfo = model.drawInfos[0];
        auto& vertexBuffer =
            at<VertexBuffer<VertexType>>()[baseDrawInfo.vertexBuffer.index];

        auto& shader = getCurrentShader();
        if (baseDrawInfo.materialPack != m_frameState.materialPack) {
            m_frameState.materialPack = baseDrawInfo.materialPack;
            auto& pack = at<MaterialPack>()[m_frameState.materialPack.index];
            shader.bindUniformBlock(pack.blockInfo());
        }

        renderer.bind();
        renderer.updateVertexBufferBinding(vertexBuffer);
        renderer.drawSingle(model, instance);
    };

    handle::Environment createEnvironmentMap(const std::filesystem::path& path,
                                             size_t cubeResolution = 1024);

    handle::Lighting createLightPack(const std::string& name,
                                     const std::vector<glm::vec3>& positions,
                                     const std::vector<glm::vec3>& intensities);

   private:
    friend class ::Application;

    Context(Window& window);
    void createDefaultMaterial();
    void createCubeRenderer();

    void cleanupCubeRenderer();

    void beginFrame();
    void drawSkybox(Environment& environment);
    void endFrame();

    template <typename Resource>
    std::vector<Resource>& at() {
        return m_resources.at<std::vector<Resource>>();
    }

    template <typename Resource>
    const std::vector<Resource>& at() const {
        return m_resources.at<std::vector<Resource>>();
    }

    template <typename Vert>
    const VertexBuffer<Vert>& getVertexBuffer(
        handle::VertexBuffer<Vert> handle) {
        auto& buffers = at<VertexBuffer<Vert>>();
        return buffers[handle.index];
    }

    template <typename Vert>
    const Model<Vert>& getDrawInfo(handle::Model<Vert> handle) {
        return m_models.at<std::vector<Model<Vert>>>()[handle.index];
    }

    template <typename Vert>
    std::unordered_map<std::string, handle::Model<Vert>>& getModelHandles() {
        return m_modelMaps
            .at<std::unordered_map<std::string, handle::Model<Vert>>>();
    }

    void loadDocumentMaterials(
        const gltf::Document& document,
        std::unordered_map<size_t, size_t>& materialPackMap);

    template <typename Vert>
    void loadDocumentModels(
        const gltf::Document& document,
        const std::unordered_map<size_t, size_t>& materialPackMap) {
        auto u32Checked = [](size_t size) {
            uint32_t u32{static_cast<uint32_t>(size)};
            if (u32 < size) {
                throw std::runtime_error("Unsigned overflow");
            }
            return u32;
        };

        auto& handles = getModelHandles<Vert>();
        auto& models = m_models.at<std::vector<Model<Vert>>>();

        const auto& materialPacks = at<MaterialPack>();
        const auto& documentModels = document.getModels<Vert>();
        if (documentModels.size()) {
            auto& vertexBuffers = at<VertexBuffer<Vert>>();
            auto& buffer = vertexBuffers.emplace_back(
                VertexBuffer{document.at<gltf::Mesh<Vert>>()});

            auto bufferIndex = u32Checked(vertexBuffers.size() - 1);
            for (auto& [modelName, model] : documentModels) {
                Model<Vert> glModel{};
                for (auto& primitive : model.primitives) {
                    uint32_t packIndex{};
                    uint32_t materialIndex{};
                    if (primitive.materialIndex.has_value()) {
                        auto index = primitive.materialIndex.value();
                        packIndex = materialPackMap.at(index);
                        materialIndex =
                            materialPacks[packIndex].getMaterialIndex(index);
                    }

                    auto offsets = buffer.getOffset(primitive.meshIndex);
                    glModel.drawInfos.emplace_back(
                        DrawInfo<Vert>{bufferIndex, packIndex, materialIndex,
                                       offsets.baseVertex, offsets.indexPointer,
                                       offsets.numIndices, offsets.mode});
                }

                models.push_back(glModel);
                handles.emplace(document.name() + "."s + modelName,
                                handle::Model<Vert>(models.size() - 1));
            }
        }
    }

    Shader& useShader(handle::Shader handle);
    Shader& getCurrentShader();

    inline static GLuint currentVertexArray{GL_NONE};
    Window& window;

    TypeMap<std::vector<Shader>, std::vector<Texture>, std::vector<Environment>,
            std::vector<MaterialPack>, std::vector<LightPack>,
            std::vector<VertexBuffer<RigidVertex>>,
            std::vector<VertexBuffer<SkinVertex>>,
            std::vector<VertexBuffer<DebugVertex>>>
        m_resources;

    TypeMap<std::vector<Model<RigidVertex>>, std::vector<Model<SkinVertex>>,
            std::vector<Model<DebugVertex>>>
        m_models;

    TypeMap<std::unordered_map<std::string, handle::Model<RigidVertex>>,
            std::unordered_map<std::string, handle::Model<SkinVertex>>,
            std::unordered_map<std::string, handle::Model<DebugVertex>>>
        m_modelMaps;

    TypeMap<Renderer<RigidVertex, glm::mat4>, Renderer<RigidVertex, glm::vec3>,
            Renderer<SkinVertex, glm::mat4>, Renderer<glm::vec3, DebugInstance>,
            Renderer<DebugVertex, DebugInstance>>
        m_renderes;

    std::unordered_map<std::string, handle::Shader> m_shaderMap;
    std::unordered_map<std::string, handle::Environment> m_environmentMap;
    std::unordered_map<std::string, handle::Lighting> m_lightingMap;

    CubeRenderer m_cubeRenderer;
    QuadRenderer m_quadRenderer;

    struct FrameState {
        glm::mat4 currentCameraMatrix;
        glm::vec3 currentCameraPosition;

        handle::Environment environment{handle::Environment::null()};
        handle::Shader shader{handle::Shader::null()};
        handle::MaterialPack materialPack{handle::MaterialPack::null()};
        handle::Lighting lighting{handle::Lighting::null()};
    } m_frameState;

    Texture m_nullTexture;
    Texture m_brdfMap;
};

}  // namespace gl