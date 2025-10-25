#pragma once

#include <glad/glad.h>

#include <filesystem>
#include <unordered_map>
#include <vector>

#include "rupture/graphics/camera.h"
#include "rupture/graphics/gl/context.inl"
#include "rupture/graphics/gl/pipeline.h"
#include "rupture/graphics/gl/environment.h"
#include "rupture/graphics/gl/framebuffer.h"
#include "rupture/graphics/gl/light.h"
#include "rupture/graphics/gl/material_pack.h"
#include "rupture/graphics/gl/model.h"
#include "rupture/graphics/gl/renderer/cube.h"
#include "rupture/graphics/gl/renderer/mesh.h"
#include "rupture/graphics/gl/renderer/quad.h"
#include "rupture/graphics/gl/shader.h"
#include "rupture/graphics/gl/texture.h"
#include "rupture/graphics/gl/uniform.h"
#include "rupture/graphics/gl/vertex_buffer.h"
#include "rupture/graphics/gltf/document.h"
#include "rupture/graphics/gltf/material.h"
#include "rupture/graphics/gltf/mesh.h"
#include "rupture/graphics/gltf/texture.h"
#include "rupture/graphics/window.h"
#include "rupture/typemap.h"
#include "rupture/utility.h"

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

    gl::handle::Shader loadShader(
        const std::filesystem::path& shaderDir);
    void loadShaders(const std::vector<std::filesystem::path>& shaderDirs);
    void loadDocument(const gltf::Document& document);

    handle::Shader getShader(const std::string& name) {
        return handleMap<handle::Shader>().at(name);
    }

    template <typename Vert>
    handle::Model<Vert> getModel(const std::string& name) {
        return handleMap<handle::Model<Vert>>().at(name);
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

        auto& handles = handleMap<handle::Model<Vert>>();
        auto& glModels = resourceStorage<std::vector<Model<Vert>>>();
        auto& vertexBuffers = resourceStorage<VertexBuffer<Vert>>();

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

    Shader& setShaderState(handle::Shader handle);
    void setEnvironment(handle::Environment handle);
    void setLighting(handle::Lighting handle);

    template <typename VertexType, typename InstanceType>
    void drawImmediate(handle::Model<VertexType> modelHandle,
                       const InstanceType& instance) {
        if (m_frameState.shader == handle::Shader::null()) {
            throw std::logic_error("Shader not set for current frame");
        }
        auto& renderer =
            m_meshRenderes.at<MeshRenderer<VertexType, InstanceType>>();

        auto& model = getDrawInfo(modelHandle);
        auto& baseDrawInfo = model.drawInfos[0];
        auto& vertexBuffer =
            getVertexBuffer<VertexType>(baseDrawInfo.vertexBuffer);

        auto& shader = getCurrentShader();
        if (baseDrawInfo.materialPack != m_frameState.materialPack) {
            m_frameState.materialPack = baseDrawInfo.materialPack;
            auto& pack = resourceStorage<MaterialPack>()[m_frameState.materialPack.index];
            shader.bindUniformBlock(pack.blockInfo());
        }

        renderer.bind();
        renderer.updateVertexBufferBinding(vertexBuffer);
        renderer.drawSingle(model, instance);
    };

    template <typename VertexType, typename InstanceType>
    void drawDeferred(handle::Model<VertexType> modelHandle,
                      const InstanceType& instance) {
        auto& model = getDrawInfo(modelHandle);
        auto& baseDrawInfo = model.drawInfos[0];
        auto commandIndex = std::make_pair(baseDrawInfo.vertexBuffer,
                                           baseDrawInfo.materialPack);
        auto& commandBufferMap =
            m_commands.at<CommandBufferMap<VertexType, InstanceType>>();
        if (commandBufferMap.find(commandIndex) == commandBufferMap.end()) {
            commandBufferMap.insert(
                {commandIndex, CommandBuffer<VertexType, InstanceType>{}});
        }
        auto& renderer =
            m_meshRenderes.at<MeshRenderer<VertexType, InstanceType>>();
        auto& commandBuffer = commandBufferMap.at(commandIndex);
        commandBuffer.push_back(renderer.prepareCommand(model, instance));
    };

    handle::Environment createEnvironmentMap(const std::filesystem::path& path,
                                             size_t cubeResolution = 1024);

    handle::Lighting createLightPack(const std::string& name,
                                     const std::vector<glm::vec3>& positions,
                                     const std::vector<glm::vec3>& intensities);

   private:
    friend class ::Application;

    friend class ::Application;

    Context(Window& window);
    void createDefaultMaterial();
    void createCubeRenderer();
    void createCommandBuffers();
    void flushCommandBuffers();

    void cleanupCubeRenderer();

    void beginFrame();
    void drawSkybox(Environment& environment);
    void endFrame();

    template <typename Resource>
    std::vector<Resource>& resourceStorage() {
        return m_resources.at<std::vector<Resource>>();
    }

    template <typename Resource>
    const std::vector<Resource>& resourceStorage() const {
        return m_resources.at<std::vector<Resource>>();
    }

    template <typename Resource>
    NamedResourceMap<Resource>& handleMap() {
        return m_handles.at<NamedResourceMap<Resource>>();
    }

    template <typename Resource>
    const NamedResourceMap<Resource>& handleMap() const {
        return m_handles.at<NamedResourceMap<Resource>>();
    }

    template <typename Vert>
    const VertexBuffer<Vert>& getVertexBuffer(
        handle::VertexBuffer<Vert> handle) {
        auto& buffers = resourceStorage<VertexBuffer<Vert>>();
        return buffers[handle.index];
    }

    template <typename Vert>
    const Model<Vert>& getDrawInfo(handle::Model<Vert> handle) {
        return resourceStorage<Model<Vert>>()[handle.index];
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

        auto& handles = handleMap<handle::Model<Vert>>();
        auto& models = resourceStorage<Model<Vert>>();

        const auto& materialPacks = resourceStorage<MaterialPack>();
        const auto& documentModels = document.getModels<Vert>();
        if (documentModels.size()) {
            auto& vertexBuffers = resourceStorage<VertexBuffer<Vert>>();
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

    void bindMaterialPack(handle::MaterialPack handle) {
        if (handle != m_frameState.materialPack) {
            m_frameState.materialPack = handle;
            auto& pack = resourceStorage<
                MaterialPack>()[m_frameState.materialPack.index];
            getCurrentShader().bindUniformBlock(pack.blockInfo());
        }
    }

    template <typename VertexType, typename InstanceType>
    void bindVertexBuffer(handle::VertexBuffer<VertexType> handle) {
        auto& renderer =
            m_meshRenderes.at<MeshRenderer<VertexType, InstanceType>>();
        auto& vertexBuffer =
            resourceStorage<VertexBuffer<VertexType>>()[handle.index];
        renderer.updateVertexBufferBinding(vertexBuffer);
    }

    template <typename VertexType, typename InstanceType>
    void processCommands() {
        if (m_frameState.shader == handle::Shader::null()) {
            throw std::logic_error("Shader not set for current frame");
        }
        auto& commands =
            m_commands.at<CommandBufferMap<VertexType, InstanceType>>();
        auto& renderer =
            m_meshRenderes.at<MeshRenderer<VertexType, InstanceType>>();
        renderer.bind();
        for (const auto& [commandIndex, commandBuffer] : commands) {
            auto [vertexBuffer, materialPack] = commandIndex;
            bindVertexBuffer<VertexType, InstanceType>(vertexBuffer);
            bindMaterialPack(materialPack);
            renderer.drawInstanced(commandBuffer);
        }
    };

    Window& window;

    ResourcesStorage m_resources;
    NamedHandleMap m_handles;

    MeshRenderers m_meshRenderes;
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

    Commands m_commands;
    TestPipeline m_pipeline;
};

}  // namespace gl