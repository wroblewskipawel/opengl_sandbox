#pragma once

#define GLFW_INCLUDE_NONE

#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include <array>
#include <string>
#include <unordered_map>
#include <vector>

#include "rupture/graphics/camera.h"
#include "rupture/graphics/gl/context.h"
#include "rupture/graphics/gl/framebuffer.h"
#include "rupture/graphics/gl/renderer.h"
#include "rupture/graphics/gl/uniform.h"
#include "rupture/graphics/window.h"

const uint32_t DEFAULT_WINDOW_WIDTH = 1024;
const uint32_t DEFAULT_WINDOW_HEIGHT = 768;

using namespace std::string_literals;

class Application {
   public:
    Application(const std::string& title = "Application"s,
                uint32_t window_width = DEFAULT_WINDOW_WIDTH,
                uint32_t window_height = DEFAULT_WINDOW_HEIGHT);
    Application(const Application&) = delete;

    Application& operator=(const Application&) = delete;
    Application& operator=(Application&&) = delete;

    ~Application(){};

    void run();

   protected:
    void quit() { m_isRunning = false; }

    virtual void loadResources(gl::Context& context) = 0;
    virtual void setupInputCallbacks(gl::Window& window) = 0;
    virtual void draw(gl::Context& context, double dTime) = 0;
    virtual void update(double dTime) = 0;

   private:
    gl::Window m_window;
    gl::Context m_context;

    bool m_isRunning{false};
};
