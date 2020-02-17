#pragma once

#include <glm/glm.hpp>

#include "ray.h"

constexpr float epsilon = std::numeric_limits<float>().epsilon();

namespace intersections {
    bool ray_sphere(const Ray ray, const glm::vec4 sphere) {
        const glm::vec3 diff = ray.origin - glm::vec3(sphere);
        const float b = glm::dot(ray.direction, diff);
        const float c = glm::dot(diff, diff) - sphere.w * sphere.w;
        float t = b * b - c;
        
        if (t > 0.0) {
            t = -b - glm::sqrt(t);
            if (t > 0.0)
                return true;
        }
        
        return false;
    }
    
    float ray_triangle(const Ray ray,
                       const glm::vec3 v0,
                       const glm::vec3 v1,
                       const glm::vec3 v2,
                       float& t,
                       float& u,
                       float& v) {
        glm::vec3 e1 = v1 - v0;
        glm::vec3 e2 = v2 - v0;
        
        glm::vec3 pvec = glm::cross(ray.direction, e2);
        float det = glm::dot(e1, pvec);
        
        // if determinant is zero then ray is
        // parallel with the triangle plane
        if (det > -epsilon && det < epsilon) return false;
        float invdet = 1.0/det;
        
        // calculate distance from m[0] to origin
        glm::vec3 tvec = ray.origin - v0;
        
        // u and v are the barycentric coordinates
        // in triangle if u >= 0, v >= 0 and u + v <= 1
        u = glm::dot(tvec, pvec) * invdet;
        
        // check against one edge and opposite point
        if (u < 0.0 || u > 1.0) return false;
        
        glm::vec3 qvec = glm::cross(tvec, e1);
        v = glm::dot(ray.direction, qvec) * invdet;
        
        // check against other edges
        if (v < 0.0 || u + v > 1.0) return false;
        
        //distance along the ray, i.e. intersect at o + t * d
        t = glm::dot(e2, qvec) * invdet;
        
        return true;
    }
};
