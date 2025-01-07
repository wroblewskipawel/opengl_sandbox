#pragma once

#include <glad/glad.h>

#include <filesystem>
#include <unordered_map>
#include <vector>

#include "rupture/graphics/camera.h"
#include "rupture/graphics/gl/environment.h"
#include "rupture/graphics/gl/framebuffer.h"
#include "rupture/graphics/gl/light.h"
#include "rupture/graphics/gl/material_pack.h"
#include "rupture/graphics/gl/model.h"
#include "rupture/graphics/gl/renderer.h"
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
#include "rupture/utility.h "

class Application;

namespace gl {

using Renderers =
    TypeMap<Renderer<RigidVertex, glm::mat4>, Renderer<RigidVertex, glm::vec3>,
            Renderer<SkinVertex, glm::mat4>, Renderer<glm::vec3, DebugInstance>,
            Renderer<DebugVertex, DebugInstance>>;

template <typename Vert>
using CommandIndex =
    std::pair<handle::VertexBuffer<Vert>, handle::MaterialPack>;

template <typename Vert, typename Instance>
using CommandBuffer = std::vector<RenderCommand<Vert, Instance>>;

template <typename Vert, typename Instance>
using CommandBufferMap =
    std::unordered_map<CommandIndex<Vert>, CommandBuffer<Vert, Instance>>;

using Commands = StaticTypeMap<CommandBufferMap<RigidVertex, glm::mat4>,
                               CommandBufferMap<RigidVertex, glm::vec3>,
                               CommandBufferMap<SkinVertex, glm::mat4>,
                               CommandBufferMap<glm::vec3, DebugInstance>,
                               CommandBufferMap<DebugVertex, DebugInstance>>;

class Context;

class Context {
   public:
    Context(const Context&) = delete;
    Context(Context&&) = delete;

    Context& operator=(const Context&) = delete;
    Context& operator=(Context&&) = delete;

    ~Context() = default;

    gl::handle::Shader Context::loadShader(
        const std::filesystem::path& shaderDir);
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

    Shader& setShaderState(handle::Shader handle);
    void setEnvironment(handle::Environment handle);
    void setLighting(handle::Lighting handle);

    template <typename VertexType, typename InstanceType>
    void drawImmediate(handle::Model<VertexType> modelHandle,
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

    template <typename VertexType, typename InstanceType>
    void drawDeferred(handle::Model<VertexType> modelHandle,
                      const InstanceType& instance) {
        auto& model = getDrawInfo(modelHandle);
        auto& baseDrawInfo = model.drawInfos[0];
        auto commandIndex = std::make_pair(
            baseDrawInfo.vertexBuffer, baseDrawInfo.materialPack);
        auto& commandBufferMap =
            m_commands.at<CommandBufferMap<VertexType, InstanceType>>();
        if (commandBufferMap.find(commandIndex) == commandBufferMap.end()) {
            commandBufferMap.insert({commandIndex, CommandBuffer<VertexType, InstanceType>{}});
        }
        auto& renderer = m_renderes.at<Renderer<VertexType, InstanceType>>();
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

    void bindMaterialPack(handle::MaterialPack handle) {
        if (handle != m_frameState.materialPack) {
            m_frameState.materialPack = handle;
            auto& pack = at<MaterialPack>()[m_frameState.materialPack.index];
            getCurrentShader().bindUniformBlock(pack.blockInfo());
        }
    }

    template <typename VertexType, typename InstanceType>
    void bindVertexBuffer(handle::VertexBuffer<VertexType> handle) {
        auto& renderer = m_renderes.at<Renderer<VertexType, InstanceType>>();
        auto& vertexBuffer = at<VertexBuffer<VertexType>>()[handle.index];
        renderer.updateVertexBufferBinding(vertexBuffer);
    }

    template <typename VertexType, typename InstanceType>
    void processCommands() {
        if (m_frameState.shader == handle::Shader::null()) {
            throw std::logic_error("Shader not set for current frame");
        }
        auto& commands =
            m_commands.at<CommandBufferMap<VertexType, InstanceType>>();
        auto& renderer = m_renderes.at<Renderer<VertexType, InstanceType>>();
        renderer.bind();
        for (const auto& [commandIndex, commandBuffer] : commands) {
            auto [vertexBuffer, materialPack] = commandIndex;
            bindVertexBuffer<VertexType, InstanceType>(vertexBuffer);
            bindMaterialPack(materialPack);
            renderer.drawInstanced(commandBuffer);
        }
    };

    inline static GLuint currentVertexArray{GL_NONE};
    Window& window;

    TypeMap<std::vector<Shader>, std::vector<Texture>, std::vector<Environment>,
            std::vector<MaterialPack>, std::vector<LightPack>,
            std::vector<VertexBuffer<RigidVertex>>,
            std::vector<VertexBuffer<SkinVertex>>,
            std::vector<VertexBuffer<DebugVertex>>,
            std::vector<VertexBuffer<glm::vec3>>>
        m_resources;

    TypeMap<std::vector<Model<RigidVertex>>, std::vector<Model<SkinVertex>>,
            std::vector<Model<DebugVertex>>, std::vector<Model<glm::vec3>>>
        m_models;

    TypeMap<std::unordered_map<std::string, handle::Model<RigidVertex>>,
            std::unordered_map<std::string, handle::Model<SkinVertex>>,
            std::unordered_map<std::string, handle::Model<DebugVertex>>,
            std::unordered_map<std::string, handle::Model<glm::vec3>>>
        m_modelMaps;

    Renderers m_renderes;

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

    Commands m_commands;
};

}  // namespace gl