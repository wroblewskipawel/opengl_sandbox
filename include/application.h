#pragma once

#define GLFW_INCLUDE_NONE

#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include <array>
#include <string>
#include <unordered_map>
#include <vector>

#include "graphics/camera.h"
#include "graphics/gl/context.h"
#include "graphics/gl/framebuffer.h"
#include "graphics/gl/renderer.h"
#include "graphics/gl/uniform.h"
#include "graphics/window.h"

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

    virtual void loadResources() = 0;
    virtual void draw(double dTime) = 0;
    virtual void update(double dTime) = 0;

   protected:
    gl::Window m_window;
    gl::Context m_context;

    std::vector<std::unique_ptr<Camera>> m_cameras;
    std::vector<gl::handle::Shader> m_shaders;
    std::vector<gl::handle::Environment> m_environments;
    std::vector<gl::handle::Lighting> m_lightPacks;
    std::vector<gl::handle::Model<RigidVertex>> m_rigidModels;

    size_t m_activeCamera;
    bool m_isRunning;
};

class Demo : public Application {
   public:
    Demo(const std::string& title = "PBR Demo"s,
         uint32_t window_width = DEFAULT_WINDOW_WIDTH,
         uint32_t window_height = DEFAULT_WINDOW_HEIGHT);

   private:
    void loadResources() override;
    void draw(double dTime) override;
    void update(double dTime) override;
};