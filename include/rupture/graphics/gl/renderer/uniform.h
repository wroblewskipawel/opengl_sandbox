#pragma once

#include <string>

using namespace std::string_literals;

namespace renderer {

namespace uniforms {

namespace cube {
const std::string PROJECTION{"CubeProjection"s};
const std::string ENV_2D_TEXTURE{"equirectangular_map"s};
const std::string ENV_CUBE_TEXTURE{"environment_map"s};
const std::string MODEL{"model"s};
const std::string ROUGHNESS{"roughness"s};
const std::string TEXTURE("image"s);
}  // namespace cube

namespace quad {
const std::string TEXTURE("image"s);
}

}  // namespace uniforms

}  // namespace renderer
