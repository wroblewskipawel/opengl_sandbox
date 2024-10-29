#pragma once

#define GLFW_INCLUDE_NONE

#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include <functional>
#include <glm/glm.hpp>
#include <iostream>
#include <optional>
#include <random>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include "rupture/graphics/camera.h"
#include "rupture/graphics/gl/shader.h"
#include "rupture/graphics/window.inl"

class Application;

namespace gl {

class Window {
   public:
    Window(const Window&) = delete;
    Window(Window&&) = delete;

    Window& operator=(const Window&) = delete;
    Window& operator=(Window&&) = delete;

    ~Window() { glfwTerminate(); };

    uint32_t width() const { return m_width; }
    uint32_t height() const { return m_height; }

    void handleInput(double dTime) {
        m_dTime = dTime;

        if (!m_hasFocus) {
            glfwWaitEvents();
        } else {
            glfwPollEvents();
            for (auto& [key, functions] : keyPressFunctions) {
                if (glfwGetKey(window, key) == GLFW_PRESS) {
                    for (auto& f : functions) {
                        f();
                    }
                }
            }
            for (auto& [button, functions] : mousePressFunctions) {
                if (glfwGetMouseButton(window, button) == GLFW_PRESS) {
                    for (auto& f : functions) {
                        f();
                    }
                }
            }
            glfwGetCursorPos(window, &m_cursorPos.x, &m_cursorPos.y);
            if (cameraCallbacks.keyState.has_value()) {
                cameraCallbacks.keyState.value()();
            }
            if (cameraCallbacks.cursorState.has_value()) {
                cameraCallbacks.cursorState.value()();
            }
        }
    };

    int getKeyState(int key) const { return glfwGetKey(window, key); }
    int getMouseButtonState(int button) const {
        return glfwGetMouseButton(window, button);
    }
    double getFrameDeltaTime() const { return m_dTime; }
    glm::dvec2 getCursorPos() const { return m_cursorPos; }

    void setCursorState(bool enabled) {
        glfwSetInputMode(window, GLFW_CURSOR,
                         enabled ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
        if (!enabled) m_cursorPos = glm::dvec2{0.0, 0.0};
    }
    void setCursorPosition(glm::dvec2 pos) {
        glfwSetCursorPos(window, pos.x, pos.y);
    }

    void setActiveCamera(Camera& camera) {
        if (cameraCallbacks.focus.has_value()) {
            cameraCallbacks.focus.value()(GLFW_FALSE);
        }

        cameraCallbacks.cursor = camera.cursorPosCallback(*this);
        cameraCallbacks.keyState = camera.keyStateFunction(*this);
        cameraCallbacks.cursorState = camera.cursorPosFunction(*this);
        cameraCallbacks.focus = camera.focusCallback(*this);
        cameraCallbacks.key = camera.keyCallbacks();
        cameraCallbacks.scroll = camera.scrollCallback();
        cameraCallbacks.resize = camera.resizeCallback();

        cameraCallbacks.camera = &camera;

        camera.m_callbackCleanup = [this, &camera]() mutable {
            if (cameraCallbacks.camera == &camera) {
                cameraCallbacks.camera = nullptr;
                cameraCallbacks.keyState = std::nullopt;
                cameraCallbacks.key = std::nullopt;
                cameraCallbacks.scroll = std::nullopt;
                cameraCallbacks.cursor = std::nullopt;
                cameraCallbacks.focus = std::nullopt;
                cameraCallbacks.cursorState = std::nullopt;
                cameraCallbacks.resize = std::nullopt;
            }
        };

        if (cameraCallbacks.focus.has_value()) {
            cameraCallbacks.focus.value()(GLFW_TRUE);
        }
    };

    void clear() { glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); }
    void display() { glfwSwapBuffers(window); }

    void registerKeyPressFunction(int key, KeyPressFunction&& function) {
        auto scancode = glfwGetKeyScancode(key);
        if (scancode != -1) {
            keyPressFunctions[scancode].push_back(std::move(function));
        }
    }

    void registerMousePressFunction(int button, KeyPressFunction&& function) {
        mousePressFunctions[button].push_back(std::move(function));
    }

    void registerKeyEventCallback(int key, KeyCallback&& callback) {
        auto scancode = glfwGetKeyScancode(key);
        if (scancode != -1) {
            keyCallbacks[scancode].push_back(std::move(callback));
        }
    }

    void registerMouseButtonCallback(int button, KeyCallback&& callback) {
        mouseCallbacks[button].push_back(std::move(callback));
    }

    void registerFocusCallback(FocusCallback&& callback) {
        focusCallbacks.push_back(std::move(callback));
    }

    void registerCloseCallback(CloseCallback&& callback) {
        closeCallbacks.push_back(std::move(callback));
    }

    void registerScrollCallback(ScrollCallback&& callback) {
        scrollCallbacks.push_back(std::move(callback));
    }

    void registerCursorPosCallback(CursorPosCallback&& callback) {
        cursorCallbacks.push_back(std::move(callback));
    }

    void registerResizeCallback(ResizeCallback&& callback) {
        resizeCallbacks.push_back(std::move(callback));
    }

    glm::mat4 getCameraMatrix() const {
        if (cameraCallbacks.camera) {
            return cameraCallbacks.camera->matrix();
        } else {
            throw std::logic_error("Active camera not set for current window");
        }
    }

    glm::vec3 getCameraPosition() const {
        if (cameraCallbacks.camera) {
            return cameraCallbacks.camera->position();
        } else {
            throw std::logic_error("Active camera not set for current window");
        }
    }

   private:
    friend class ::Application;
    friend class Context;

    Window(uint32_t width, uint32_t height, const std::string& title)
        : m_width{width}, m_height{height} {
        if (window) {
            throw std::logic_error(
                "Attempted creation of second OpenGL context");
        }
        glfwSetErrorCallback((GLFWerrorfun)glfwErrorCallback);
        if (glfwInit() != GLFW_TRUE) {
            throw std::runtime_error("Failed to initialize GLFW");
        };
        try {
            glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
            glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
            glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
            window = glfwCreateWindow(m_width, m_height, title.c_str(), nullptr,
                                      nullptr);
            if (!window) {
                throw std::runtime_error("Failed to create GLFW window");
            }
            glfwMakeContextCurrent(window);
            if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
                throw std::runtime_error("Failed to initialize OpenGL");
            };

            glfwSetWindowCloseCallback(window, glfwCloseEventCallback);
            glfwSetWindowFocusCallback(window, glfwFocusEventCallback);
            glfwSetFramebufferSizeCallback(window, glfwResizeCallback);

            resizeCallbacks.push_back([this](int width, int height) mutable {
                m_width = width;
                m_height = height;
                glViewport(0, 0, width, height);
            });
            focusCallbacks.push_back([this](int focus) {
                enableUserInputOnFocus(focus == GLFW_TRUE);
            });
            enableUserInputOnFocus(true);
            glfwFocusWindow(window);

            m_dTime = 0.0;

            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glEnable(GL_DEPTH_TEST);
            glEnable(GL_CULL_FACE);
            glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
            glEnable(GL_FRAMEBUFFER_SRGB);
            glFrontFace(GL_CCW);
            glCullFace(GL_BACK);
            glClearDepth(1.0f);
            glDepthFunc(GL_LEQUAL);

#ifndef NDEBUG
            glEnable(GL_DEBUG_OUTPUT);
            glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
            glDebugMessageCallback(glDebugOutput, nullptr);
            glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0,
                                  nullptr, GL_TRUE);
#endif

        } catch (const std::exception&) {
            glfwTerminate();
            throw;
        }
    };

    void enableUserInputOnFocus(bool hasFocus) {
        if (hasFocus && !m_hasFocus) {
            glfwGetCursorPos(window, &m_cursorPos.x, &m_cursorPos.y);
            enableInputCallbacks();
            m_hasFocus = true;
        } else if (!hasFocus && m_hasFocus) {
            disableInputCallbacks();
            m_hasFocus = false;
        }
    }

    void enableInputCallbacks() {
        glfwSetKeyCallback(window, glfwKeyEventCallback);
        glfwSetMouseButtonCallback(window, glfwMouseEventCallback);
        glfwSetCursorPosCallback(window, glfwCursorPosCallback);
        glfwSetScrollCallback(window, glfwMouseScrollCallback);
    }
    void disableInputCallbacks() {
        glfwSetKeyCallback(window, nullptr);
        glfwSetMouseButtonCallback(window, nullptr);
        glfwSetCursorPosCallback(window, nullptr);
        glfwSetScrollCallback(window, nullptr);
    }

    inline static GLFWwindow* window{nullptr};

    glm::dvec2 m_cursorPos;
    double m_dTime;
    uint32_t m_width;
    uint32_t m_height;
    bool m_hasFocus{false};

    inline static std::unordered_map<int, std::vector<KeyPressFunction>>
        keyPressFunctions{};
    inline static std::unordered_map<int, std::vector<KeyPressFunction>>
        mousePressFunctions{};
    inline static std::unordered_map<int, std::vector<KeyCallback>>
        keyCallbacks{};
    inline static std::unordered_map<int, std::vector<KeyCallback>>
        mouseCallbacks{};
    inline static std::vector<ScrollCallback> scrollCallbacks{};
    inline static std::vector<CursorPosCallback> cursorCallbacks{};
    inline static std::vector<FocusCallback> focusCallbacks{};
    inline static std::vector<CloseCallback> closeCallbacks{};
    inline static std::vector<ResizeCallback> resizeCallbacks{};

    inline static struct CameraCallbacks {
        const Camera* camera;
        std::optional<std::unordered_map<int, KeyCallback>> key;
        std::optional<KeyPressFunction> keyState;
        std::optional<CursorPosFunction> cursorState;
        std::optional<ScrollCallback> scroll;
        std::optional<CursorPosCallback> cursor;
        std::optional<FocusCallback> focus;
        std::optional<ResizeCallback> resize;
    } cameraCallbacks;

    static void glfwErrorCallback(int err, const char* desc) {
        std::cerr << "[GLFW Error][" << err << "]" << desc << '\n';
    };

    static void glfwKeyEventCallback(GLFWwindow* window, int key, int scancode,
                                     int action, int mods) {
        auto callbacks = keyCallbacks.find(scancode);
        if (callbacks != keyCallbacks.end()) {
            for (auto& c : callbacks->second) c(action, mods);
        }
    }
    static void glfwMouseEventCallback(GLFWwindow* window, int button,
                                       int action, int mods) {
        auto callbacks = mouseCallbacks.find(button);
        if (callbacks != mouseCallbacks.end()) {
            for (auto& c : callbacks->second) c(action, mods);
        }
    }

    static void glfwMouseScrollCallback(GLFWwindow* window, double x_offset,
                                        double y_offset) {
        for (auto& c : scrollCallbacks) c(y_offset);
        if (cameraCallbacks.scroll.has_value()) {
            cameraCallbacks.scroll.value()(y_offset);
        }
    };
    static void glfwCursorPosCallback(GLFWwindow* window, double x_pos,
                                      double y_pos) {
        for (auto& c : cursorCallbacks) c(x_pos, y_pos);
        if (cameraCallbacks.cursor.has_value()) {
            cameraCallbacks.cursor.value()(x_pos, y_pos);
        }
    };
    static void glfwCloseEventCallback(GLFWwindow* window) {
        for (auto& c : closeCallbacks) c();
    }
    static void glfwFocusEventCallback(GLFWwindow* window, int focused) {
        for (auto& c : focusCallbacks) c(focused);
        if (cameraCallbacks.focus.has_value()) {
            cameraCallbacks.focus.value()(focused);
        }
    }
    static void glfwResizeCallback(GLFWwindow* window, int width, int height) {
        for (auto& c : resizeCallbacks) c(width, height);
        if (cameraCallbacks.resize.has_value()) {
            cameraCallbacks.resize.value()(width, height);
        }
    }

    static void APIENTRY glDebugOutput(GLenum source, GLenum type,
                                       unsigned int id, GLenum severity,
                                       GLsizei length, const char* message,
                                       const void* userParam) {
        // ignore non-significant error/warning codes
        if (id == 131169 || id == 131185 || id == 131218 || id == 131204)
            return;

        std::cout << "---------------" << std::endl;
        std::cout << "Debug message (" << id << "): " << message << std::endl;

        switch (source) {
            case GL_DEBUG_SOURCE_API:
                std::cout << "Source: API";
                break;
            case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
                std::cout << "Source: Window System";
                break;
            case GL_DEBUG_SOURCE_SHADER_COMPILER:
                std::cout << "Source: Shader Compiler";
                break;
            case GL_DEBUG_SOURCE_THIRD_PARTY:
                std::cout << "Source: Third Party";
                break;
            case GL_DEBUG_SOURCE_APPLICATION:
                std::cout << "Source: Application";
                break;
            case GL_DEBUG_SOURCE_OTHER:
                std::cout << "Source: Other";
                break;
        }
        std::cout << std::endl;

        switch (type) {
            case GL_DEBUG_TYPE_ERROR:
                std::cout << "Type: Error";
                break;
            case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
                std::cout << "Type: Deprecated Behaviour";
                break;
            case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
                std::cout << "Type: Undefined Behaviour";
                break;
            case GL_DEBUG_TYPE_PORTABILITY:
                std::cout << "Type: Portability";
                break;
            case GL_DEBUG_TYPE_PERFORMANCE:
                std::cout << "Type: Performance";
                break;
            case GL_DEBUG_TYPE_MARKER:
                std::cout << "Type: Marker";
                break;
            case GL_DEBUG_TYPE_PUSH_GROUP:
                std::cout << "Type: Push Group";
                break;
            case GL_DEBUG_TYPE_POP_GROUP:
                std::cout << "Type: Pop Group";
                break;
            case GL_DEBUG_TYPE_OTHER:
                std::cout << "Type: Other";
                break;
        }
        std::cout << std::endl;

        switch (severity) {
            case GL_DEBUG_SEVERITY_HIGH:
                std::cout << "Severity: high";
                break;
            case GL_DEBUG_SEVERITY_MEDIUM:
                std::cout << "Severity: medium";
                break;
            case GL_DEBUG_SEVERITY_LOW:
                std::cout << "Severity: low";
                break;
            case GL_DEBUG_SEVERITY_NOTIFICATION:
                std::cout << "Severity: notification";
                break;
        }
        std::cout << std::endl;
        std::cout << std::endl;
    }
};

}  // namespace gl