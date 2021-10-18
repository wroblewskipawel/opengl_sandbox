#define GLFW_INCLUDE_NONE
#include "application.h"

#include <glad/glad.h>

#include <exception>
#include <iostream>

Application::Application(const char* title_, uint32_t window_width,
                         uint32_t window_height)
    : title{std::string{title_}},
      WINDOW_WIDTH{window_width},
      WINDOW_HEIGHT{window_height},
      is_running{false} {
    try {
        if (!glfwInit()) {
            throw std::runtime_error{"Failed to initialize GLFW"};
        }
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
        window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, title.c_str(),
                                  nullptr, nullptr);
        if (!window) {
            throw std::runtime_error{"Failed to create GLFW window"};
        }
        glfwMakeContextCurrent(window);
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
            throw std::runtime_error{"Failed to initialize OpenGL context"};
        };
    } catch (std::exception&) {
        glfwTerminate();
        throw;
    }
    is_running = true;
}

Application::~Application() { glfwTerminate(); }

int Application::run() {
    while (is_running) {
        if (glfwWindowShouldClose(window)) {
            is_running = false;
            break;
        }
        glfwPollEvents();
    }
    return EXIT_SUCCESS;
}