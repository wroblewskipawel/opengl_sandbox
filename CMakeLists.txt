cmake_minimum_required(VERSION 3.16)
project(rupture VERSION 0.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

set(JSON_BuildTests OFF CACHE INTERNAL "")
set(JSON_Install OFF CACHE INTERNAL "")

add_compile_definitions(GLM_FORCE_QUAT_DATA_XYZW)

add_subdirectory(${CMAKE_SOURCE_DIR}/extern/glm/)
add_subdirectory(${CMAKE_SOURCE_DIR}/extern/glad/)
add_subdirectory(${CMAKE_SOURCE_DIR}/extern/glfw/)
add_subdirectory(${CMAKE_SOURCE_DIR}/extern/json/)

file(GLOB_RECURSE SOURCE ${CMAKE_SOURCE_DIR}/src/*.cpp)
file(GLOB_RECURSE HEADERS ${CMAKE_SOURCE_DIR}/include/*.h)

list(APPEND EXTERNAL_LIBS glad glfw glm nlohmann_json::nlohmann_json)
if(UNIX)
    list(APPEND EXTERNAL_LIBS X11 pthread Xrandr Xi)
endif()

find_package(OpenGL REQUIRED)
list(APPEND EXTERNAL_LIBS ${OpenGL_LIBRARIES})

add_library(${PROJECT_NAME} STATIC ${SOURCE} ${HEADERS})
target_link_libraries(${PROJECT_NAME} PUBLIC ${EXTERNAL_LIBS})

target_include_directories(
    ${PROJECT_NAME}
    PUBLIC
    "${CMAKE_SOURCE_DIR}/include/"
    "${CMAKE_SOURCE_DIR}/extern/json/include/"
    "${CMAKE_SOURCE_DIR}/extern/fx-gltf/"
    "${CMAKE_SOURCE_DIR}/extern/glfw/include/"
    "${CMAKE_SOURCE_DIR}/extern/glm/"
    "${CMAKE_SOURCE_DIR}/extern/stb/"
    "${CMAKE_SOURCE_DIR}/extern/magic_enum/"
    "${CMAKE_SOURCE_DIR}/extern/glad/include/"
)

set(SHADER_DIR "${CMAKE_SOURCE_DIR}/shaders/")
set(ASSETS_DIR "${CMAKE_SOURCE_DIR}/assets/")
target_compile_definitions(
    ${PROJECT_NAME}
    PUBLIC
    SHADER_DIR="${SHADER_DIR}"
    ASSETS_DIR="${ASSETS_DIR}"
)

add_subdirectory(${CMAKE_SOURCE_DIR}/examples/)
