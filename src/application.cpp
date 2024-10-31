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
    : m_window{window_width, window_height, title}, m_context{m_window} {
    m_window.registerCloseCallback([this]() mutable { m_isRunning = false; });
}

void Application::run() {
    loadResources(m_context);
    setupInputCallbacks(m_window);

    m_window.snapCursorToCenter();
    m_window.enableUserInputOnFocus(true);

    std::chrono::steady_clock clock{};
    auto lastTime = clock.now();
    m_isRunning = true;
    while (m_isRunning) {
        auto currenTime = clock.now();
        double dTime =
            std::chrono::duration<double>(currenTime - lastTime).count();
        lastTime = currenTime;

        m_window.handleInput(dTime);

        m_context.beginFrame();
        draw(m_context, dTime);
        m_context.endFrame();

        update(dTime);
    }
}
