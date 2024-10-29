#pragma once

#include <string>

namespace shader {

inline std::string nameOf(const std::string& path) {
    return path.substr(path.find_last_of('/') + 1);
}

namespace pbr {
const std::string ROOT_DIRECFORY{SHADER_DIR + "pbr/"s};
const std::string COOK_TORRANCE{ROOT_DIRECFORY + "cook-torrance"s};
}  // namespace pbr

namespace debug {
const std::string ROOT_DIRECFORY{SHADER_DIR + "debug/"s};
const std::string LINE{ROOT_DIRECFORY + "line"s};
}  // namespace debug

namespace non_pbr {
const std::string ROOT_DIRECFORY{SHADER_DIR + "non_pbr/"s};
const std::string DIFFUSE{ROOT_DIRECFORY + "diffuse"s};
}  // namespace non_pbr

namespace animated {
const std::string ROOT_DIRECFORY{SHADER_DIR + "animated/"s};
const std::string DIFFUSE{ROOT_DIRECFORY + "diffuse"s};
}  // namespace animated

namespace core {
const std::string ROOT_DIRECFORY{SHADER_DIR + "core/"s};
const std::string DEPTH_CUBE{ROOT_DIRECFORY + "depth_cube"s};
const std::string COLOR_CUBE{ROOT_DIRECFORY + "color_cube"s};
const std::string IRRADIANCE_MAP{ROOT_DIRECFORY + "irradiance_map"s};
const std::string SPECULAR_MAP{ROOT_DIRECFORY + "specular_map"s};
const std::string BRDF_MAP{ROOT_DIRECFORY + "brdf_map"s};
const std::string SKYBOX{ROOT_DIRECFORY + "skybox"s};
const std::string TEXTURED_QUAD{ROOT_DIRECFORY + "textured_quad"s};
}  // namespace core

}  // namespace shader
