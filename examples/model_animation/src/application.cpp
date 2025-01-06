#include "application.h"

#include <rupture/graphics/assets/assets.h>
#include <rupture/graphics/shaders/shaders.h>

Demo::Demo(const std::string& title, uint32_t window_width,
           uint32_t window_height)
    : Application{title, window_width, window_height},
      m_document{gltf::Document{"assets/CesiumMan/glTF/CesiumMan.gltf"s}} {};

void Demo::loadResources(gl::Context& context) {
    context.loadDocument(m_document);
    m_shader = context.loadShader(shader::animated::DIFFUSE);
    m_skinModel = context.getModel<SkinVertex>("CesiumMan.Cesium_Man");
    m_environment = context.createEnvironmentMap(assets::env_map::NEWPORT_LOFT);

    const auto& skin = m_document.at<gltf::Skin>()[0];
    m_animationDuration = skin.animationDuration(animationIndex);
}

void Demo::setupInputCallbacks(gl::Window& window) {
    m_camera = window.addCamera<TopViewCamera>(window, false);
    window.setActiveCamera(m_camera);
    window.registerKeyEventCallback(
        GLFW_KEY_RIGHT, [this](int action, int mods) mutable {
            if (action == GLFW_PRESS && m_animDebugMode) {
                m_elapsedTime += m_animDebugStep;
                std::cout << "Elapsed Time: " << m_elapsedTime << std::endl;
            }
        });
    window.registerKeyEventCallback(
        GLFW_KEY_LEFT, [this](int action, int mods) mutable {
            if (action == GLFW_PRESS && m_animDebugMode) {
                m_elapsedTime -= m_animDebugStep;
                std::cout << "Elapsed Time: " << m_elapsedTime << std::endl;
            }
        });
    window.registerKeyEventCallback(
        GLFW_KEY_UP, [this](int action, int mods) mutable {
            if (action == GLFW_PRESS && m_animDebugMode) {
                m_animDebugStep += 0.0005;
                std::cout << "Animation Step: " << m_animDebugStep << std::endl;
            }
        });
    window.registerKeyEventCallback(
        GLFW_KEY_DOWN, [this](int action, int mods) mutable {
            if (action == GLFW_PRESS && m_animDebugMode) {
                m_animDebugStep -= 0.0005;
                std::cout << "Animation Step: " << m_animDebugStep << std::endl;
            }
        });
    window.registerKeyEventCallback(
        GLFW_KEY_SLASH, [this](int action, int mods) mutable {
            if (action == GLFW_PRESS) {
                m_animDebugMode = !m_animDebugMode;
                if (m_animDebugMode) {
                    std::cout << "Enabled animDebugMode" << std::endl;
                } else {
                    std::cout << "Disabled animDebugMode" << std::endl;
                }
            }
        });
}

void Demo::draw(gl::Context& context, double dTime) {
    context.setEnvironment(m_environment);
    context.setLighting(m_lightPack);
    context.setShaderState(m_shader).bindUniformBlock(
        m_jointUniform.blockInfo());
    context.draw(m_skinModel, glm::identity<glm::mat4>());
}

void Demo::update(double dTime) {
    if (!m_animDebugMode) {
        m_elapsedTime += dTime;
    }
    m_elapsedTime -= static_cast<uint32_t>(m_elapsedTime / m_animationDuration) * m_animationDuration;
    const auto& skin = m_document.at<gltf::Skin>()[0];
    auto jointMatrices =
        skin.jointMatrices(glm::identity<glm::mat4>(), animationIndex, m_elapsedTime);
    for (size_t index = 0; index < jointMatrices.size(); ++index) {
        m_jointBlock.value.at(index) = jointMatrices[index];
    }
    m_jointUniform.write(m_jointBlock);
}
