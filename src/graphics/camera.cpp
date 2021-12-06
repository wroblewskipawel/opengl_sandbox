#include "graphics/camera.h"

#include "graphics/window.h"

Camera::Camera(gl::Window& window, float fovYDeg, float nearClipPlane,
               float farClipPlane)
    : m_fovY{glm::radians(fovYDeg)},
      m_nearClipPlane{nearClipPlane},
      m_farClipPlane{farClipPlane},
      m_aspectRatio{static_cast<float>(window.width()) / window.height()} {
    updateProjectionMatrix();
};

std::optional<ResizeCallback> Camera::resizeCallback() {
    return [this](int width, int height) mutable {
        if (width != 0 && height != 0) {
            m_aspectRatio = static_cast<float>(width) / height;
            updateProjectionMatrix();
        };
    };
}

FirstPersonCamera::FirstPersonCamera(gl::Window& window, glm::vec3 position,
                                     glm::vec3 forward, float fovYDeg,
                                     float nearClipPlane, float farClipPlane)
    : Camera{window, fovYDeg, nearClipPlane, farClipPlane},
      m_position{position},
      m_forward{forward} {
    updateVectors();
}

std::optional<ScrollCallback> FirstPersonCamera::scrollCallback() {
    return [this](double y_offset) mutable {
        m_fovY = glm::clamp(m_fovY + SCROLL_SENSITIVITY * y_offset,
                            glm::radians(15.0), glm::radians(90.0));
        updateProjectionMatrix();
    };
}

std::optional<CursorPosCallback> FirstPersonCamera::cursorPosCallback(
    gl::Window& window) {
    return [this, &window](double x_pos, double y_pos) mutable {
        auto old_pos = window.getCursorPos();
        float x_offset = static_cast<float>(x_pos - old_pos.x);
        float y_offset = static_cast<float>(y_pos - old_pos.y);
        m_pitch = glm::clamp(m_pitch - y_offset * MOUSE_SENSITIVITY,
                             glm::radians(-89.0f), glm::radians(89.0f));
        m_yaw = m_yaw - x_offset * MOUSE_SENSITIVITY;
        updateVectors();
    };
}
std::optional<KeyPressFunction> FirstPersonCamera::keyStateFunction(
    gl::Window& window) {
    return [this, &window]() mutable {
        glm::vec3 offset{};
        if (window.getKeyState(GLFW_KEY_W) == GLFW_PRESS) offset += m_forward;
        if (window.getKeyState(GLFW_KEY_S) == GLFW_PRESS) offset -= m_forward;
        if (window.getKeyState(GLFW_KEY_A) == GLFW_PRESS) offset += m_right;
        if (window.getKeyState(GLFW_KEY_D) == GLFW_PRESS) offset -= m_right;
        if (glm::length(offset) > 0.0f) {
            offset = glm::normalize(offset) * MOVEMENT_SPEED;
            m_position +=
                offset * static_cast<float>(window.getFrameDeltaTime());
        }
    };
}

std::optional<FocusCallback> FirstPersonCamera::focusCallback(
    gl::Window& window) {
    return [&window](int focused) {
        if (focused == GLFW_TRUE) {
            window.setCursorState(false);
        } else {
            window.setCursorState(true);
        }
    };
}

TopViewCamera::TopViewCamera(gl::Window& window, glm::vec3 target,
                             float azimuth, float altitude, float radius,
                             float fovYDeg, float nearClipPlane,
                             float farClipPlane)
    : Camera{window, fovYDeg, nearClipPlane, farClipPlane},
      m_target{target},
      m_azimuth{glm::radians(azimuth)},
      m_altitude{glm::clamp(glm::radians(altitude), glm::radians(1.0f),
                            glm::radians(90.0f))},
      m_radius{glm::clamp(radius, MIN_RADIUS, MAX_RADIUS)} {
    updateVectors();
};

std::optional<ScrollCallback> TopViewCamera::scrollCallback() {
    return [this](double y_offset) mutable {
        m_radius = glm::clamp(
            m_radius + static_cast<float>(y_offset) * SCROLL_SENSITIVITY,
            MIN_RADIUS, MAX_RADIUS);
        updateVectors();
    };
}

std::optional<CursorPosCallback> TopViewCamera::cursorPosCallback(
    gl::Window& window) {
    return [this, &window](double x_pos, double y_pos) mutable {
        auto old_pos = window.getCursorPos();
        if (window.getMouseButtonState(GLFW_MOUSE_BUTTON_MIDDLE) ==
            GLFW_PRESS) {
            auto old_pos = window.getCursorPos();
            float x_offset = static_cast<float>(x_pos - old_pos.x);
            float y_offset = static_cast<float>(y_pos - old_pos.y);
            m_azimuth = m_azimuth + x_offset * MOUSE_SENSITIVITY;
            m_altitude = glm::clamp(m_altitude + y_offset * MOUSE_SENSITIVITY,
                                    glm::radians(1.0f), glm::radians(89.0f));
            updateVectors();
        }
    };
}

std::optional<KeyPressFunction> TopViewCamera::keyStateFunction(
    gl::Window& window) {
    return [this, &window]() mutable {
        glm::vec3 offset{};
        if (window.getKeyState(GLFW_KEY_W) == GLFW_PRESS) offset += m_forward;
        if (window.getKeyState(GLFW_KEY_S) == GLFW_PRESS) offset -= m_forward;
        if (window.getKeyState(GLFW_KEY_A) == GLFW_PRESS) offset += m_right;
        if (window.getKeyState(GLFW_KEY_D) == GLFW_PRESS) offset -= m_right;
        if (glm::length(offset) > 0.0f) {
            offset = glm::normalize(offset) * MOVEMENT_SPEED;
            m_target += offset * static_cast<float>(window.getFrameDeltaTime());
            updateVectors();
        }
    };
}

std::optional<CursorPosFunction> TopViewCamera::cursorPosFunction(
    gl::Window& window) {
    return [this, &window]() {
        auto cursor_pos = window.getCursorPos();
        glm::vec3 offset{};
        if (cursor_pos.x > window.width() - 1.0f) {
            offset -= m_right;
        } else if (cursor_pos.x < 1.0f) {
            offset += m_right;
        }
        if (cursor_pos.y > window.height() - 1.0f) {
            offset -= m_forward;
        } else if (cursor_pos.y < 1.0f) {
            offset += m_forward;
        }
        if (glm::length(offset) > 0.0f) {
            offset = glm::normalize(offset) * MOVEMENT_SPEED;
            m_target += offset * static_cast<float>(window.getFrameDeltaTime());
            updateVectors();
        }
        window.setCursorPosition(
            glm::clamp(cursor_pos, glm::dvec2{0.0, 0.0},
                       glm::dvec2{window.width(), window.height()}));
    };
}

std::optional<FocusCallback> TopViewCamera::focusCallback(gl::Window& window) {
    return [&window](int focused) {
        if (focused == GLFW_TRUE) {
            window.setCursorPosition(
                glm::dvec2{window.width() / 2, window.height() / 2});
        }
    };
}