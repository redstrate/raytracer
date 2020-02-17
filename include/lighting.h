#pragma once

#include <glm/glm.hpp>

namespace lighting {
    float point_light(const glm::vec3 pos, const glm::vec3 light, const glm::vec3 normal) {
        const glm::vec3 dir = light - pos;
        const float n_dot_l = glm::max(glm::dot(normal, glm::normalize(dir)), 0.0f);
        
        return n_dot_l;
    }
};
