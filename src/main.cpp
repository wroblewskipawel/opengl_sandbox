#define GLFW_INCLUDE_VULKAN

#include <iostream>
#include <stdint.h>
#include <GLFW/glfw3.h>
#include <values.h>

const uint32_t WINDOW_WIDTH = 1024;
const uint32_t WINDOW_HEIGHT = 728;
const char* APPLICATION_TITLE = "Rupture";


int main() {
    if(!glfwInit()) {
        std::cout << "Failed to initialize GLFW" << '\n';
        return EXIT_FAILURE;
    }
    glfwWindowHint(GLFW_NO_API, GLFW_TRUE);
    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, APPLICATION_TITLE, nullptr, nullptr);
    if(!window) {
        std::cout << "Failed to create window" << "\n";
        return EXIT_FAILURE;
    }

    VkApplicationInfo app_info{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app_info.apiVersion = VK_API_VERSION_1_1;
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pApplicationName = APPLICATION_TITLE;

    uint32_t extension_count{};
    const char** extension_names = glfwGetRequiredInstanceExtensions(&extension_count);
    VkInstanceCreateInfo create_info{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    create_info.pApplicationInfo = &app_info;
    create_info.enabledExtensionCount = extension_count;
    create_info.ppEnabledExtensionNames = extension_names;

    VkInstance instance{};
    if(vkCreateInstance(&create_info, nullptr, &instance) != VK_SUCCESS) {
        std::cout << "Failed to create Vulkan Instance" << '\n';
        glfwTerminate();
        return EXIT_FAILURE;
    }

    bool is_running{true};
    while(is_running) {
        glfwPollEvents();
        if(glfwWindowShouldClose(window)) {
            is_running = false;
        }
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return EXIT_SUCCESS;
}