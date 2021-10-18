#pragma once
#include <GLFW/glfw3.h>
#include <stdint.h>

#include <string>

const uint32_t DEFAULT_WINDOW_WIDTH = 1024;
const uint32_t DEFAULT_WINDOW_HEIGHT = 728;

class Application {
   public:
    Application(const char* title, uint32_t window_width = DEFAULT_WINDOW_WIDTH,
                uint32_t window_height = DEFAULT_WINDOW_HEIGHT);
    ~Application();
    Application(Application const&) = delete;
    Application& operator=(Application const&) = delete;

    int run();

   private:
    std::string title;
    const uint32_t WINDOW_WIDTH;
    const uint32_t WINDOW_HEIGHT;
    bool is_running;

    GLFWwindow* window;
};