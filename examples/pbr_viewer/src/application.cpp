#include "application.h"

#include <rupture/graphics/assets/assets.h>
#include <rupture/graphics/shaders/shaders.h>

Demo::Demo(const std::string& title, uint32_t window_width,
           uint32_t window_height)
    : Application{title, window_width, window_height} {};

void Demo::loadResources(gl::Context& context) {
    context.loadShaders({shader::pbr::COOK_TORRANCE});
    m_shaders.emplace_back(
        context.getShader(shader::nameOf(shader::pbr::COOK_TORRANCE)));

    gltf::Document waterBottle{"assets/WaterBottle/glTF/WaterBottle.gltf"s};
    context.loadDocument(waterBottle);

    m_rigidModels.emplace_back(
        context.getModel<RigidVertex>("WaterBottle.WaterBottle"));

    m_environments.push_back(
        context.createEnvironmentMap(assets::env_map::NEWPORT_LOFT));

    std::vector<glm::vec3> lightPositions{glm::vec3{2.0f, 1.0f, 1.0f},
                                          glm::vec3{-1.0f, -2.0f, 0.0f},
                                          glm::vec3{-1.0f, 1.5f, 0.0f}};

    std::vector<glm::vec3> lightIntensities{glm::vec3{23.47f, 41.31f, 20.79f},
                                            glm::vec3{13.47f, 21.31f, 50.79f},
                                            glm::vec3{33.47f, 21.31f, 70.79f}};

    m_lightPacks.emplace_back(context.createLightPack(
        "Scene lighting"s, lightPositions, lightIntensities));
}

void Demo::setupInputCallbacks(gl::Window& window) {
    window.registerKeyEventCallback(GLFW_KEY_Q,
                                    [this](int action, int mods) mutable {
                                        if (action == GLFW_PRESS) {
                                            quit();
                                        }
                                    });
    m_cameras.reserve(2);
    m_cameras.emplace_back(window.addCamera<TopViewCamera>(window, false));
    m_cameras.emplace_back(window.addCamera<FirstPersonCamera>(window));
    window.setActiveCamera(m_cameras[m_activeCamera]);

    for (auto camera : m_cameras) {
        window.getCamera(camera).lookAt(glm::vec3{0.5f, 0.5f, 0.5f},
                                        glm::vec3{0.0f, 0.0f, 0.0f});
    }

    window.registerKeyEventCallback(
        GLFW_KEY_LEFT_BRACKET, [this, &window](int action, int mods) mutable {
            if (action == GLFW_PRESS) {
                m_activeCamera = (m_activeCamera - 1) % m_cameras.size();
                window.setActiveCamera(m_cameras[m_activeCamera]);
            }
        });
    window.registerKeyEventCallback(
        GLFW_KEY_RIGHT_BRACKET, [this, &window](int action, int mods) mutable {
            if (action == GLFW_PRESS) {
                m_activeCamera = (m_activeCamera + 1) % m_cameras.size();
                window.setActiveCamera(m_cameras[m_activeCamera]);
            }
        });
}

void Demo::draw(gl::Context& context, double dTime) {
    context.setEnvironment(m_environments[0]);
    context.setLighting(m_lightPacks[0]);
    context.setShaderState(m_shaders[0]);
    context.drawDeferred(m_rigidModels[0],
                         glm::rotate(glm::mat4{1.0f}, glm::radians(90.0f),
                                     glm::vec3{1.0f, 0.0f, 0.0f}));
}

void Demo::update(double dTime) {}
