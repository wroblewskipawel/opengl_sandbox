#include "application.h"

#include <rupture/graphics/assets/assets.h>
#include <rupture/graphics/shaders/shaders.h>

Demo::Demo(const std::string& title, uint32_t window_width,
           uint32_t window_height)
    : Application{title, window_width, window_height} {};

void Demo::loadResources() {
    m_context.loadShaders({shader::pbr::COOK_TORRANCE});
    m_shaders.emplace_back(
        m_context.getShader(shader::nameOf(shader::pbr::COOK_TORRANCE)));

    gltf::Document waterBottle{
        "assets/gltf/WaterBottle/glTF/WaterBottle.gltf"s};
    m_context.loadDocument(waterBottle);

    m_rigidModels.emplace_back(
        m_context.getModel<RigidVertex>("WaterBottle.WaterBottle"));

    m_environments.push_back(
        m_context.createEnvironmentMap(assets::env_map::NEWPORT_LOFT));

    std::vector<glm::vec3> lightPositions{glm::vec3{2.0f, 1.0f, 1.0f},
                                          glm::vec3{-1.0f, -2.0f, 0.0f},
                                          glm::vec3{-1.0f, 1.5f, 0.0f}};

    std::vector<glm::vec3> lightIntensities{glm::vec3{23.47f, 41.31f, 20.79f},
                                            glm::vec3{13.47f, 21.31f, 50.79f},
                                            glm::vec3{33.47f, 21.31f, 70.79f}};

    m_lightPacks.emplace_back(m_context.createLightPack(
        "Scene lighting"s, lightPositions, lightIntensities));
}

void Demo::draw(double dTime) {
    m_context.setEnvironment(m_environments[0]);
    m_context.setLighting(m_lightPacks[0]);
    m_context.setShaderState(m_shaders[0]);
    m_context.draw(m_rigidModels[0],
                   glm::rotate(glm::mat4{1.0f}, glm::radians(90.0f),
                               glm::vec3{1.0f, 0.0f, 0.0f}));
}

void Demo::update(double dTime) {}
