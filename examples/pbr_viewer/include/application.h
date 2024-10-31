#pragma once

#include <string>
#include <rupture/application.h>

class Demo : public Application {
   public:
    Demo(const std::string& title = "PbrViewer"s,
         uint32_t window_width = DEFAULT_WINDOW_WIDTH,
         uint32_t window_height = DEFAULT_WINDOW_HEIGHT);

   private:
    void loadResources(gl::Context& context) override;
    void setupInputCallbacks(gl::Window& window) override;
    void draw(gl::Context& context, double dTime) override;
    void update(double dTime) override;

    std::vector<handle::Camera> m_cameras;
    std::vector<gl::handle::Shader> m_shaders;
    std::vector<gl::handle::Environment> m_environments;
    std::vector<gl::handle::Lighting> m_lightPacks;
    std::vector<gl::handle::Model<RigidVertex>> m_rigidModels;

    size_t m_activeCamera{0};
    
};
