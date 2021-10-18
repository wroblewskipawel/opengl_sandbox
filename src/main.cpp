#define GLFW_INCLUDE_VULKAN

#include <exception>
#include <iostream>

#include "application.h"

int main() {
    try {
        Application app{"Rupture"};
        app.run();
    } catch (std::exception& e) {
        std::cout << e.what() << '\n';
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
