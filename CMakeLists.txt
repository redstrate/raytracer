cmake_minimum_required(VERSION 3.17)
project(raytracer)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(GLM REQUIRED)
find_package(SDL2 REQUIRED)

add_subdirectory(extern)

add_executable(raytracer
    include/camera.h
    include/intersections.h
    include/lighting.h
    include/ray.h
    include/image.h
    include/tiny_obj_loader.h
    include/scene.h
    src/main.cpp
    src/scene.cpp)
target_include_directories(raytracer PUBLIC include PRIVATE ${GLM_INCLUDE_DIR})
target_link_libraries(raytracer PUBLIC stb SDL2::Core imgui glad)
set_target_properties(raytracer PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)
