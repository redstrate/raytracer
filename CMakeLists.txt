cmake_minimum_required(VERSION 2.6)
project(raytracer)

find_package(GLM REQUIRED)

add_subdirectory(extern)

add_executable(raytracer
    include/camera.h
    include/intersections.h
    include/lighting.h
    include/ray.h
    include/image.h
    include/tiny_obj_loader.h
    include/scene.h
    src/main.cpp)
target_include_directories(raytracer PUBLIC include)
target_link_libraries(raytracer PRIVATE stb glm)
set_target_properties(raytracer PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)
