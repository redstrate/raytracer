#pragma once

#include <glm/glm.hpp>

struct Ray {
    Ray(const glm::vec3 origin, const glm::vec3 direction) : origin(origin), direction(direction) {}

    glm::vec3 origin, direction;
};
