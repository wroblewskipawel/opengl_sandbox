#include <exception>
#include <iostream>

#include "application.h"

int main() {
    try {
        Demo app{};
        app.run();
    } catch (std::exception& e) {
        std::cout << e.what() << '\n';
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
