#pragma once

#include <rupture/application.h>

#include <string>

class Demo : public Application {
   public:
    Demo(const std::string& title = "TileMap"s,
         uint32_t window_width = DEFAULT_WINDOW_WIDTH,
         uint32_t window_height = DEFAULT_WINDOW_HEIGHT);

   private:
    void loadResources(gl::Context& context) override;
    void setupInputCallbacks(gl::Window& window) override;
    void draw(gl::Context& context, double dTime) override;
    void update(double dTime) override;
};
