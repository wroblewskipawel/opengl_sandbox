cmake_minimum_required(VERSION 3.16)
project(model_animation VERSION 0.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

file(GLOB_RECURSE SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
file(GLOB_RECURSE HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h)

add_executable(${PROJECT_NAME} ${SOURCE} ${HEADERS})

target_link_libraries(${PROJECT_NAME} PRIVATE rupture)

target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include/"
    "${CMAKE_SOURCE_DIR}/include/interface/"
)

copy_directory(
    ${PROJECT_NAME}
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/
    ${CMAKE_CURRENT_BINARY_DIR}/assets/
)
