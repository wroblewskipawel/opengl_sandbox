#include "application.h"

#include <rupture/graphics/assets/assets.h>
#include <rupture/graphics/shaders/shaders.h>

Demo::Demo(const std::string& title, uint32_t window_width,
           uint32_t window_height)
    : Application{title, window_width, window_height} {};

void Demo::loadResources(gl::Context& context) {}

void Demo::setupInputCallbacks(gl::Window& window) {};

void Demo::draw(gl::Context& context, double dTime) {}

void Demo::update(double dTime) {}
