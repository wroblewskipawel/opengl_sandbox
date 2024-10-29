#pragma once

#define GLM_FORCE_RADIANS

#include <algorithm>
#include <array>
#include <functional>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <optional>

#include "rupture/graphics/window.inl"

class Camera {
   public:
    Camera(gl::Window& window, float fovYDeg = 60.0f,
           float nearClipPlane = 0.001f, float farClipPlane = 1000.0f);

    Camera(const Camera&) = delete;
    Camera(Camera&&) = default;

    Camera& operator=(const Camera&) = delete;
    Camera& operator=(Camera&&) = delete;

    virtual ~Camera() {
        if (m_callbackCleanup.has_value()) {
            m_callbackCleanup.value()();
        }
    };

    glm::mat4 matrix() const { return proj * view(); };
    virtual glm::vec3 position() const = 0;
    virtual void setPosition(glm::vec3 position) = 0;
    virtual void lookAt(glm::vec3 eye, glm::vec3 center) = 0;

   protected:
    const float MOVEMENT_SPEED{1.0f};
    const float MOUSE_SENSITIVITY{0.005f};
    const float SCROLL_SENSITIVITY{0.1f};
    const glm::vec3 up{0.0f, 0.0f, 1.0f};

    friend class gl::Window;

    std::optional<ResizeCallback> resizeCallback();

    virtual std::optional<std::unordered_map<int, KeyCallback>> keyCallbacks() {
        return std::nullopt;
    }
    virtual std::optional<ScrollCallback> scrollCallback() {
        return std::nullopt;
    }
    virtual std::optional<CursorPosCallback> cursorPosCallback(
        gl::Window& window) {
        return std::nullopt;
    }
    virtual std::optional<KeyPressFunction> keyStateFunction(
        gl::Window& window) {
        return std::nullopt;
    }
    virtual std::optional<CursorPosFunction> cursorPosFunction(
        gl::Window& window) {
        return std::nullopt;
    }
    virtual std::optional<FocusCallback> focusCallback(gl::Window& window) {
        return std::nullopt;
    }

    glm::mat4 proj;
    float m_fovY;
    float m_nearClipPlane;
    float m_farClipPlane;
    float m_aspectRatio;

    void updateProjectionMatrix() {
        proj = glm::perspective(m_fovY, m_aspectRatio, m_nearClipPlane,
                                m_farClipPlane);
    };

    virtual glm::mat4 view() const = 0;

    std::array<glm::vec4, 6> clipPlanes() const {
        auto m = glm::transpose(matrix());
        return {-(m[3] + m[0]), -(m[3] - m[0]), -(m[3] + m[1]),
                -(m[3] - m[1]), -(m[3] + m[2]), -(m[3] - m[1])};
    }

   private:
    std::optional<std::function<void(void)>> m_callbackCleanup;
};

class FirstPersonCamera : public Camera {
   public:
    FirstPersonCamera(gl::Window& window,
                      glm::vec3 position = glm::vec3{0.0f, 0.0f, 0.0f},
                      glm::vec3 forward = glm::vec3{0.0f, 1.0f, 0.0f},
                      float fovYDeg = 60.0f, float nearClipPlane = 0.001f,
                      float farClipPlane = 1000.0f);

    FirstPersonCamera(const FirstPersonCamera&) = delete;
    FirstPersonCamera(FirstPersonCamera&&) = default;

    FirstPersonCamera& operator=(const FirstPersonCamera&) = delete;
    FirstPersonCamera& operator=(FirstPersonCamera&&) = delete;

    ~FirstPersonCamera() override = default;

    glm::vec3 position() const override { return m_position; }
    void setPosition(glm::vec3 position) override { m_position = position; }
    void lookAt(glm::vec3 eye, glm::vec3 center) override {
        m_position = eye;
        auto direction = glm::normalize(center - eye);
        m_pitch = glm::asin(direction.z);
        m_yaw = glm::atan(direction.x, direction.y);
        updateVectors();
    }

   private:
    glm::vec3 m_position;
    glm::vec3 m_forward;
    glm::vec3 m_right;

    float m_yaw;
    float m_pitch;

    bool active;

    inline glm::mat4 view() const override {
        return glm::lookAt(m_position, m_position + m_forward, up);
    }

    void updateVectors() {
        m_forward =
            glm::vec3{glm::cos(m_yaw) * glm::cos(m_pitch),
                      glm::sin(m_yaw) * glm::cos(m_pitch), glm::sin(m_pitch)};
        m_right = glm::normalize(glm::cross(up, m_forward));
    }

    std::optional<ScrollCallback> scrollCallback() override;
    std::optional<CursorPosCallback> cursorPosCallback(
        gl::Window& window) override;
    std::optional<KeyPressFunction> keyStateFunction(
        gl::Window& window) override;
    std::optional<FocusCallback> focusCallback(gl::Window& window) override;
};

class TopViewCamera : public Camera {
   public:
    TopViewCamera(gl::Window& window,
                  glm::vec3 target = glm::vec3{0.0f, 0.0f, 0.0f},
                  float azimuth = 45.0f, float altitude = 0.0f,
                  float radius = 10.0f, float fovYDeg = 60.0f,
                  float nearClipPlane = 0.001f, float farClipPlane = 1000.0f);

    TopViewCamera(const TopViewCamera&) = delete;
    TopViewCamera(TopViewCamera&&) = default;

    TopViewCamera& operator=(const TopViewCamera&) = delete;
    TopViewCamera& operator=(TopViewCamera&&) = delete;

    ~TopViewCamera() override = default;

    glm::vec3 position() const override { return m_position; }
    void setPosition(glm::vec3 target) override { m_target = target; }
    void lookAt(glm::vec3 eye, glm::vec3 center) override {
        m_position = eye;
        m_target = center;
        m_radius = glm::clamp(glm::length(m_position - m_target), MIN_RADIUS,
                              MAX_RADIUS);
        auto direction = glm::normalize(m_position - m_target);
        m_altitude = glm::asin(direction.z);
        m_azimuth = glm::atan(direction.x, direction.y);
        m_right = glm::normalize(glm::cross(direction, up));
        m_forward = glm::normalize(glm::cross(m_right, up));
    }

   private:
    const float MIN_RADIUS{0.1f};
    const float MAX_RADIUS{1000.0f};

    glm::vec3 m_position;
    glm::vec3 m_right;
    glm::vec3 m_forward;
    glm::vec3 m_target;

    float m_azimuth;
    float m_altitude;
    float m_radius;

    glm::mat4 view() const override {
        return glm::lookAt(m_position, m_target, up);
    }

    void updateVectors() {
        m_position =
            m_target + (glm::vec3{glm::cos(m_altitude) * glm::sin(m_azimuth),
                                  glm::cos(m_altitude) * glm::cos(m_azimuth),
                                  glm::sin(m_altitude)} *
                        m_radius);
        m_right = glm::normalize(glm::cross(up, m_target - m_position));
        m_forward = glm::normalize(glm::cross(m_right, up));
    }

    std::optional<ScrollCallback> scrollCallback() override;
    std::optional<CursorPosCallback> cursorPosCallback(
        gl::Window& window) override;
    std::optional<CursorPosFunction> cursorPosFunction(
        gl::Window& window) override;
    std::optional<KeyPressFunction> keyStateFunction(
        gl::Window& window) override;
    std::optional<FocusCallback> focusCallback(gl::Window& window) override;
};