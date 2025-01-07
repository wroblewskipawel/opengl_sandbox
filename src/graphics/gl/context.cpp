#include "rupture/graphics/gl/context.h"

#define GLM_FORCE_RADIANS

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <magic_enum.hpp>

using namespace magic_enum;
using namespace std::string_literals;

namespace gl {

Context::Context(Window& window)
    : window{window},
      m_nullTexture{gltf::Texture::null(), 1},
      m_brdfMap{m_quadRenderer.createBRDFMap(512, 512)} {
    createDefaultMaterial();
    createCommandBuffers();
};

gl::handle::Shader Context::loadShader(const std::filesystem::path& shaderDir) {
    auto& shaders = at<Shader>();
    shaders.emplace_back(shaderDir);
    auto shaderHandle = handle::Shader(shaders.size() - 1);
    m_shaderMap.emplace(shaderDir.stem().string(), shaderHandle);
    return shaderHandle;
}

void Context::loadShaders(
    const std::vector<std::filesystem::path>& shaderDirs) {
    auto& shaders = at<Shader>();
    for (auto& dir : shaderDirs) {
        shaders.emplace_back(dir);
        m_shaderMap.emplace(dir.stem().string(),
                            handle::Shader(shaders.size() - 1));
    }
}

void Context::loadDocument(const gltf::Document& document) {
    std::unordered_map<size_t, size_t> materialPackMap{};
    loadDocumentMaterials(document, materialPackMap);
    loadDocumentModels<RigidVertex>(document, materialPackMap);
    loadDocumentModels<SkinVertex>(document, materialPackMap);
}

void Context::createDefaultMaterial() {
    auto& textures = at<Texture>();
    auto& materialPacks = at<MaterialPack>();
    PackBuilder nullBuilder{};
    nullBuilder.setMaterialData(0, gltf::pbrMaterial::null(), 0, textures,
                                m_nullTexture);
    materialPacks.emplace_back(MaterialPack{std::move(nullBuilder)});
}

void Context::createCommandBuffers() {
    m_commands.insert<CommandBufferMap<RigidVertex, glm::mat4>>();
    m_commands.insert<CommandBufferMap<RigidVertex, glm::vec3>>();
    m_commands.insert<CommandBufferMap<SkinVertex, glm::mat4>>();
    m_commands.insert<CommandBufferMap<glm::vec3, DebugInstance>>();
    m_commands.insert<CommandBufferMap<DebugVertex, DebugInstance>>();
}

void Context::loadDocumentMaterials(
    const gltf::Document& document,
    std::unordered_map<size_t, size_t>& materialPackMap) {
    auto& textures = at<Texture>();

    auto textureIndexOffset = textures.size();
    for (const auto& texture : document.at<gltf::Texture>()) {
        textures.emplace_back(texture);
    }

    auto& materialPacks = at<MaterialPack>();
    auto materialPackIndexOffset = materialPacks.size();

    std::vector<PackBuilder> packBuilders{};
    packBuilders.emplace_back();

    const auto& documentMaterials = document.at<gltf::pbrMaterial>();
    for (size_t i{0}; i < documentMaterials.size(); i++) {
        if (packBuilders.back().full()) {
            packBuilders.emplace_back();
        }
        packBuilders.back().setMaterialData(i, documentMaterials[i],
                                            textureIndexOffset, textures,
                                            m_nullTexture);
        materialPackMap.emplace(
            i, materialPackIndexOffset + packBuilders.size() - 1);
    }
    for (auto& builder : packBuilders) {
        auto& pack =
            materialPacks.emplace_back(MaterialPack{std::move(builder)});
        pack.setTexturesResident();
    }
}

handle::Environment Context::createEnvironmentMap(
    const std::filesystem::path& path, size_t cubeResolution) {
    auto& environments = at<Environment>();

    environments.emplace_back(
        m_cubeRenderer.createEnvironmentMap(path, cubeResolution));

    auto handle = handle::Environment{environments.size() - 1};
    m_environmentMap.emplace(path.stem().string(), handle);

    return handle;
}

handle::Lighting Context::createLightPack(
    const std::string& name, const std::vector<glm::vec3>& positions,
    const std::vector<glm::vec3>& intensities) {
    auto& lighting = at<LightPack>();

    lighting.emplace_back(LightPack{positions, intensities});

    auto handle = handle::Lighting(lighting.size() - 1);
    m_lightingMap.emplace(name, handle);

    return handle;
}

void Context::beginFrame() {
    window.clear();

    m_frameState.currentCameraMatrix = window.getCameraMatrix();
    m_frameState.currentCameraPosition = window.getCameraPosition();

    m_frameState.materialPack = handle::MaterialPack::null();
    m_frameState.environment = handle::Environment::null();
    m_frameState.shader = handle::Shader::null();
    m_frameState.lighting = handle::Lighting::null();
};

void Context::endFrame() { 
    flushCommandBuffers();
    window.display();
}

void Context::flushCommandBuffers() {
    processCommands<RigidVertex, glm::mat4>();
    processCommands<RigidVertex, glm::vec3>();
    processCommands<SkinVertex, glm::mat4>();
    processCommands<glm::vec3, DebugInstance>();
    processCommands<DebugVertex, DebugInstance>();
    m_commands.forEach([](auto& commands) { commands.clear(); });
}

Shader& Context::setShaderState(handle::Shader handle) {
    auto& shader = useShader(handle);

    shader.setUniformMatrix4(Shader::CAMERA_MATRIX_UNIFORM,
                             m_frameState.currentCameraMatrix);

    if (shader.hasUniform(Shader::CAMERA_POS_UNIFORM)) {
        shader.setUniformVec3(Shader::CAMERA_POS_UNIFORM,
                              m_frameState.currentCameraPosition);
    }

    if (m_frameState.lighting != handle::Lighting::null()) {
        auto& lighPack = at<LightPack>()[m_frameState.lighting.index];
        if (shader.hasBlock(lighPack.blockName())) {
            shader.bindUniformBlock(lighPack.blockInfo());
        }
    }
    if (m_frameState.materialPack != handle::MaterialPack::null()) {
        auto& materialPack =
            at<MaterialPack>()[m_frameState.materialPack.index];
        if (shader.hasBlock(materialPack.blockName())) {
            shader.bindUniformBlock(materialPack.blockInfo());
        }
    }
    if (m_frameState.environment != handle::Environment::null()) {
        auto& environment = at<Environment>()[m_frameState.environment.index];
        auto irradianceTexture = environment.irradianceMap().texture();
        auto specularTexture = environment.specularMap().texture();

        if (shader.hasUniform(Shader::IRRADIANCE_CUBE_MAP)) {
            glBindTextureUnit(0, irradianceTexture);
            shader.setUniformI(Shader::IRRADIANCE_CUBE_MAP, 0);
        }

        if (shader.hasUniform(Shader::SPECULAR_CUBE_MAP)) {
            glBindTextureUnit(1, specularTexture);
            shader.setUniformI(Shader::SPECULAR_CUBE_MAP, 1);
        }

        if (shader.hasUniform(Shader::BRDF_MAP)) {
            glBindTextureUnit(2, m_brdfMap.texture());
            shader.setUniformI(Shader::BRDF_MAP, 2);
        }
    }
    return shader;
}

void Context::setEnvironment(handle::Environment handle) {
    m_frameState.environment = handle;
    auto& environment = at<Environment>()[m_frameState.environment.index];

    m_cubeRenderer.drawSkybox(environment, m_frameState.currentCameraMatrix,
                              m_frameState.currentCameraPosition);

    if (m_frameState.shader != handle::Shader::null()) {
        auto& shader = useShader(m_frameState.shader);
        auto irradianceTexture = environment.irradianceMap().texture();
        auto specularTexture = environment.specularMap().texture();

        glBindTextureUnit(0, irradianceTexture);
        shader.setUniformI(Shader::IRRADIANCE_CUBE_MAP, 0);

        glBindTextureUnit(1, specularTexture);
        shader.setUniformI(Shader::SPECULAR_CUBE_MAP, 1);

        glBindTextureUnit(2, m_brdfMap.texture());
        shader.setUniformI(Shader::BRDF_MAP, 2);
    }
}

void Context::setLighting(handle::Lighting handle) {
    m_frameState.lighting = handle;
    if (m_frameState.shader != handle::Shader::null()) {
        auto& lightPack = at<LightPack>()[m_frameState.lighting.index];
        auto& shaders = at<Shader>()[m_frameState.shader.index];
        shaders.bindUniformBlock(lightPack.blockInfo());
    }
}

Shader& Context::useShader(handle::Shader handle) {
    auto& shaders = at<Shader>();
    if (handle != handle::Shader::null()) {
        if (handle != m_frameState.shader) {
            m_frameState.shader = handle;
            shaders[m_frameState.shader.index].use();
            return shaders[m_frameState.shader.index];
        } else {
            return shaders[m_frameState.shader.index];
        }
    } else {
        throw std::logic_error("Invalid shader handle");
    }
}

Shader& Context::getCurrentShader() {
    auto& shaders = at<Shader>();
    if (m_frameState.shader != handle::Shader::null()) {
        return shaders[m_frameState.shader.index];
    } else {
        throw std::logic_error("Invalid shader handle");
    }
}

}  // namespace gl