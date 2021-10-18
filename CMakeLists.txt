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

add_subdirectory(${CMAKE_SOURCE_DIR}/extern/glm/)
add_subdirectory(${CMAKE_SOURCE_DIR}/extern/glfw/)
add_subdirectory(${CMAKE_SOURCE_DIR}/extern/json/)

file(GLOB_RECURSE SOURCE ${CMAKE_SOURCE_DIR}/src/*.cpp)
file(GLOB_RECURSE HEADERS ${CMAKE_SOURCE_DIR}/include/*.h)
file(GLOB_RECURSE GLAD_SOURCE ${CMAKE_SOURCE_DIR}/extern/glad/src/*.c)
file(GLOB_RECURSE GLAD_HEADERS ${CMAKE_SOURCE_DIR}/extern/glad/include/*.h)

list(APPEND EXTERNAL_LIBS glfw glm nlohmann_json::nlohmann_json)
if(UNIX)
    list(APPEND EXTERNAL_LIBS X11 pthread Xrandr Xi)
endif()

find_package(Vulkan REQUIRED)
list(APPEND EXTERNAL_LIBS ${Vulkan_LIBRARIES})

add_executable(${PROJECT_NAME} ${SOURCE} ${HEADERS} ${GLAD_SOURCE} ${GLAD_HEADERS})
target_link_libraries(${PROJECT_NAME} PRIVATE ${EXTERNAL_LIBS})

target_include_directories(
    ${PROJECT_NAME} PRIVATE
    "${Vulkan_INCLUDE_DIRS}"
    "${CMAKE_SOURCE_DIR}/include/"
    "${CMAKE_SOURCE_DIR}/extern/json/include/"
    "${CMAKE_SOURCE_DIR}/extern/fx-gltf/include/"
    "${CMAKE_SOURCE_DIR}/extern/glfw/include/"
    "${CMAKE_SOURCE_DIR}/extern/glm/"
    "${CMAKE_SOURCE_DIR}/extern/stb/"
    "${CMAKE_SOURCE_DIR}/extern/glad/include/"
)

add_custom_target(
    UPDATE_SHADERS
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/shaders/
    ${CMAKE_BINARY_DIR}/shaders/
)

add_custom_target(
    UPDATE_ASSETS
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets/
    ${CMAKE_BINARY_DIR}/assets/
)
