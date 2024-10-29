#include "rupture/application.h"

#define GLM_FORCE_RADIANS

#include <glad/glad.h>

#include <chrono>
#include <exception>
#include <future>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <vector>

#include "rupture/graphics/gl/block/std140.h"
#include "rupture/graphics/gl/light.h"
#include "rupture/graphics/gltf/document.h"
#include "rupture/graphics/shape.h"
#include "rupture/graphics/window.h"
#include "rupture/world/tile_map.h"
#include "rupture/world/tile_set.h"

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
