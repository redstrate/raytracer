#pragma once

#include "ray.h"

class Camera {
public:
    Camera() : position(glm::vec3(0)), direction(glm::vec3(0)) {}
    Camera(glm::vec3 position, glm::vec3 direction) : position(position), direction(direction) {}
    
    void look_at(glm::vec3 eye, glm::vec3 target) {
        position = eye;
        direction = glm::normalize(target - eye);
    }
    
    Ray get_ray(const int32_t x, const int32_t y, const int32_t width, const int32_t height) const {
        const glm::vec3 up = glm::vec3(0, 1, 0);
        const glm::vec3 right = glm::normalize(glm::cross(direction, up));
        
        const float h2 = height / 2.0f;
        const float w2 = width / 2.0f;
        
        const glm::vec3 ray_dir = position + (h2 / tan(glm::radians(fov) / 2)) * direction + (y - h2) * up + static_cast<float>(x - w2) * right;
        return Ray(position, ray_dir);
    }
    
    glm::vec3 position, direction;
    float fov = 45.0f;
};
