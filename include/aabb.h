#pragma once

struct AABB {
    glm::vec3 min, max;
    
    glm::vec3 center() const {
        return 0.5f * (max + min);
    }
    
    glm::vec3 extent() const {
        return max - center();
    }
    
    bool contains(const glm::vec3 point) const {
        return glm::all(glm::lessThan(point, max)) && glm::all(glm::greaterThan(point, min));
    }
    
    bool inside(const AABB extent) const {
        return(max.x > extent.min.x &&
               min.x < extent.max.x &&
               max.y > extent.min.y &&
               min.y < extent.max.y &&
               max.z > extent.min.z &&
               min.z < extent.max.z);
    }
    
    bool contains(const Ray& ray) const {
        const float t1 = (min.x - ray.origin.x) / ray.direction.x;
        const float t2 = (max.x - ray.origin.x) / ray.direction.x;
        const float t3 = (min.y - ray.origin.y) / ray.direction.y;
        const float t4 = (max.y - ray.origin.y) / ray.direction.y;
        const float t5 = (min.z - ray.origin.z) / ray.direction.z;
        const float t6 = (max.z - ray.origin.z) / ray.direction.z;
        
        const float tmin = std::min(std::max(std::min(t1, t2), std::min(t3, t4)), std::min(t5, t6));
        const float tmax = std::min(std::min(std::max(t1, t2), std::max(t3, t4)), std::max(t5, t6));
        
        // if tmax < 0, ray (line) is intersecting AABB, but whole AABB is behing us
        //if(tmax < 0)
        //    return false;
        
        // if tmin > tmax, ray doesn't intersect AABB
        if(tmin > tmax)
            return false;
        
        if(tmin < 0.0f)
            return true;

        return true;
    }
};
