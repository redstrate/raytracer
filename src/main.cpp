#include <iostream>
#include <limits>
#include <future>
#include <vector>
#include <glm/glm.hpp>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include "intersections.h"
#include "camera.h"
#include "image.h"
#include "lighting.h"
#include "scene.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

// scene information
constexpr int32_t width = 512, height = 512;
constexpr glm::vec3 light_position = glm::vec3(5);
const Camera camera = [] {
    Camera camera;
    camera.look_at(glm::vec3(0, 0, 4), glm::vec3(0));
    
    return camera;
}();
constexpr std::string_view model_path = "suzanne.obj";
constexpr glm::vec3 model_color = glm::vec3(1.0f, 1.0f, 1.0f);

// internal variables
constexpr float light_bias = 0.01f;
constexpr int32_t tile_size = 32;
constexpr int32_t num_tiles_x = width / tile_size;
constexpr int32_t num_tiles_y = height / tile_size;

// globals
Scene scene = {};
Image<glm::ivec4, width, height> colors = {};

bool calculate_tile(const int32_t from_x, const int32_t to_width, const int32_t from_y, const int32_t to_height) {
    for (int32_t y = from_y; y < (from_y + to_height); y++) {
        for (int32_t x = from_x; x < (from_x + to_width); x++) {
            Ray ray_camera = camera.get_ray(x, y, width, height);
            
            if (auto hit = test_scene(ray_camera, scene)) {
                const float diffuse = lighting::point_light(hit->position, light_position, hit->normal);
                
                //shadow calculation
                float shadow = 0.0f;
                if(glm::dot(light_position - hit->position, hit->normal) > 0) {
                    const glm::vec3 light_dir = glm::normalize(light_position - hit->position);
                    
                    const Ray shadow_ray(hit->position + (hit->normal * light_bias), light_dir);
                    
                    if(test_scene(shadow_ray, scene))
                        shadow = 1.0f;
                }
                
                const glm::vec3 finalColor = model_color * diffuse * (1.0f - shadow);
                colors.get(x, y) = glm::ivec4(finalColor * 255.0f, 255.0f);
            }
        }
    }
    
    return true;
}

int main(int, char*[]) {
    if(!scene.load_from_file(model_path))
        return -1;
    
    std::vector<std::future<bool>> futures;
    for(int32_t y = 0; y < num_tiles_y; y++)
        for(int32_t x = 0; x < num_tiles_x; x++)
            futures.push_back(std::async(std::launch::async, calculate_tile, x * tile_size, tile_size, y * tile_size, tile_size));
    
    for(auto& future : futures)
        future.wait();
    
    std::cout << "rendering done!!" << std::endl;
    
    uint8_t pixels[width * height * 3] = {};
    int i = 0;
    for (int32_t y = height - 1; y >= 0; y--) {
        for (int32_t x = 0; x < width; x++) {
            const glm::ivec4 c = colors.get(x, y);
            pixels[i++] = c.r;
            pixels[i++] = c.g;
            pixels[i++] = c.b;
        }
    }
    
    if(stbi_write_png("output.png", width, height, 3, pixels, width * 3) != 1)
        return -1;
    
    return 0;
}
