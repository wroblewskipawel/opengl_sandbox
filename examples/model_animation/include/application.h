#pragma once

#include <rupture/application.h>

#include <string>

class Demo : public Application {
   public:
    Demo(const std::string& title = "ModelAnimation"s,
         uint32_t window_width = DEFAULT_WINDOW_WIDTH,
         uint32_t window_height = DEFAULT_WINDOW_HEIGHT);

   private:
    void loadResources(gl::Context& context) override;
    void setupInputCallbacks(gl::Window& window) override;
    void draw(gl::Context& context, double dTime) override;
    void update(double dTime) override;

    handle::Camera m_camera{handle::Camera::null()};
    gl::handle::Shader m_shader{gl::handle::Shader::null()};
    gl::handle::Environment m_environment{gl::handle::Environment::null()};
    gl::handle::Lighting m_lightPack{gl::handle::Lighting::null()};
    gl::handle::Model<SkinVertex> m_skinModel{
        gl::handle::Model<SkinVertex>::null()};

    bool m_animDebugMode{false};
    double m_animationDuration{0.0}; 
    double m_animDebugStep{0.005};
    double m_elapsedTime{0};
    gltf::Document m_document;
    gl::std140::Block<std::array<glm::mat4, 20>> m_jointBlock;
    std::vector<glm::mat4> m_jointMatrices;
    gl::ShaderUniform m_jointUniform{"JointMatrices"};

    constexpr static uint32_t animationIndex{0};
};
