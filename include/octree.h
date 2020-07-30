#pragma once

#include "aabb.h"

constexpr int max_contained_types = 16;
constexpr int max_octree_depth = 2;

constexpr auto child_pattern = {
    glm::vec3(+1, -1, -1),
    glm::vec3(+1, -1, +1),
    glm::vec3(+1, +1, -1),
    glm::vec3(+1, +1, +1),
    glm::vec3(-1, -1, -1),
    glm::vec3(-1, -1, +1),
    glm::vec3(-1, +1, -1),
    glm::vec3(-1, +1, +1),
};

template<typename UnderlyingType>
struct Node {
    using NodeType = Node<UnderlyingType>;
    
    AABB extent;
    int depth = 0;
    
    std::vector<UnderlyingType> contained_objects;
    
    std::array<std::unique_ptr<NodeType>, 8> children;
    bool is_split = false;
    
    void add(UnderlyingType& object, const AABB extent) {
        if(is_split) {
            for(auto& child : children) {
                if(extent.inside(child->extent))
                    child->add(object, extent);
            }
        } else {
            contained_objects.push_back(object);
            
            if(contained_objects.size() >= max_contained_types && depth < max_octree_depth)
                split();
        }
    }
    
    void split() {
        is_split = true;
        
        const auto center = extent.center();
        const auto split_size = extent.extent().x / 2.0f;
        
        int i = 0;
        for(auto& point : child_pattern) {
            auto child = std::make_unique<NodeType>();
            child->depth = depth + 1;
            
            const auto position = center + point * split_size;
            child->extent.min = glm::vec3(position.x - split_size, position.y - split_size, position.z - split_size);
            child->extent.max = glm::vec3(position.x + split_size, position.y + split_size, position.z + split_size);
            
            children[i++] = std::move(child);
        }
        
        for(auto& object : contained_objects) {
            for(auto& child : children) {
                if(object.extent.inside(child->extent))
                    child->add(object, extent);
            }
        }
        
        contained_objects.clear();
    }
};

template<typename UnderlyingType>
struct Octree {
    using NodeType = Node<UnderlyingType>;
    
    NodeType root;
    
    Octree(const glm::vec3 min, const glm::vec3 max) {
        root = NodeType();
        root.extent.min = min;
        root.extent.max = max;
    }
    
    void add(UnderlyingType& object, const AABB extent) {
        root.add(object, extent);
    }
};
