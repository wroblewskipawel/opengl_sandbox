#pragma once

#include <string>
#include <rupture/application.h>

class Demo : public Application {
   public:
    Demo(const std::string& title = "TileMap"s,
         uint32_t window_width = DEFAULT_WINDOW_WIDTH,
         uint32_t window_height = DEFAULT_WINDOW_HEIGHT);

   private:
    void loadResources() override;
    void draw(double dTime) override;
    void update(double dTime) override;
};
