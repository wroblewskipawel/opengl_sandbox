#include "application.h"

#define GLM_FORCE_RADIANS

#include <glad/glad.h>

#include <chrono>
#include <exception>
#include <future>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <vector>

#include "graphics/gl/block/std140.h"
#include "graphics/gl/light.h"
#include "graphics/gltf/document.h"
#include "graphics/shape.h"
#include "graphics/window.h"
#include "world/tile_map.h"
#include "world/tile_set.h"

using namespace std::string_literals;

Application::Application(const std::string& title, uint32_t window_width,
                         uint32_t window_height)
    : m_window{window_width, window_height, title},
      m_context{m_window},
      m_isRunning{false} {
    m_window.registerCloseCallback([this]() mutable { m_isRunning = false; });
    m_window.registerKeyEventCallback(GLFW_KEY_Q,
                                      [this](int action, int mods) mutable {
                                          if (action == GLFW_PRESS) {
                                              m_isRunning = false;
                                          }
                                      });
    m_cameras.emplace_back(std::make_unique<FirstPersonCamera>(m_window));
    m_cameras.emplace_back(std::make_unique<TopViewCamera>(m_window));
    m_activeCamera = 1;
    m_window.setActiveCamera(*m_cameras[m_activeCamera]);
    for (auto& camera : m_cameras) {
        camera->lookAt(glm::vec3{0.5f, 0.5f, 0.5f},
                       glm::vec3{0.0f, 0.0f, 0.0f});
    }
    m_window.registerKeyEventCallback(
        GLFW_KEY_LEFT_BRACKET, [this](int action, int mods) mutable {
            if (action == GLFW_PRESS) {
                m_activeCamera = (m_activeCamera - 1) % m_cameras.size();
                m_window.setActiveCamera(*m_cameras[m_activeCamera]);
            }
        });
    m_window.registerKeyEventCallback(
        GLFW_KEY_RIGHT_BRACKET, [this](int action, int mods) mutable {
            if (action == GLFW_PRESS) {
                m_activeCamera = (m_activeCamera + 1) % m_cameras.size();
                m_window.setActiveCamera(*m_cameras[m_activeCamera]);
            }
        });

    m_isRunning = true;
}

void Application::run() {
    loadResources();

    std::chrono::steady_clock clock{};
    auto lastTime = clock.now();

    while (m_isRunning) {
        auto currenTime = clock.now();
        double dTime =
            std::chrono::duration<double>(currenTime - lastTime).count();
        lastTime = currenTime;

        m_window.handleInput(dTime);

        m_context.beginFrame();
        draw(dTime);
        m_context.endFrame();

        update(dTime);
    }
}

Demo::Demo(const std::string& title, uint32_t window_width,
           uint32_t window_height)
    : Application{title, window_width, window_height} {};

void Demo::loadResources() {
    m_context.loadShaders({"shaders/pbr/cook-torrance"});
    m_shaders.emplace_back(m_context.getShader("cook-torrance"));

    gltf::Document waterBottle{
        "assets/gltf/WaterBottle/glTF/WaterBottle.gltf"s};
    m_context.loadDocument(waterBottle);

    m_rigidModels.emplace_back(
        m_context.getModel<RigidVertex>("WaterBottle.WaterBottle"));

    m_environments.push_back(m_context.createEnvironmentMap(
        "assets/textures/Newport_Loft_Ref.hdr"s));

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
