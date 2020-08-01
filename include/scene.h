#pragma once

#include <optional>
#include <glm/glm.hpp>
#include <array>
#include <iostream>
#include <random>

#include <tiny_obj_loader.h>

#include "ray.h"
#include "intersections.h"
#include "lighting.h"
#include "octree.h"

constexpr glm::vec3 light_position = glm::vec3(5);
constexpr float light_bias = 0.01f;
constexpr int max_depth = 2;
inline int num_indirect_samples = 4;

struct TriangleBox {
    uint64_t vertice_index = 0;
    tinyobj::mesh_t* mesh;
    AABB extent;
};

struct Object;

glm::vec3 fetch_position(const Object& object, const tinyobj::mesh_t& mesh, const int32_t index, const int32_t vertex);
glm::vec3 fetch_normal(const Object& object, const tinyobj::mesh_t& mesh, const int32_t index, const int32_t vertex);

struct Object {
    glm::vec3 position = glm::vec3(0);
    glm::vec3 color = glm::vec3(1);
    
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    
    std::unique_ptr<Octree<TriangleBox>> octree;

    void create_octree() {
        octree = std::make_unique<Octree<TriangleBox>>(glm::vec3(-2), glm::vec3(2));
        
        for(auto& shape : shapes) {
            for(size_t i = 0; i < shape.mesh.num_face_vertices.size(); i++) {
                const glm::vec3 v0 = fetch_position(*this, shape.mesh, i, 0);
                const glm::vec3 v1 = fetch_position(*this, shape.mesh, i, 1);
                const glm::vec3 v2 = fetch_position(*this, shape.mesh, i, 2);
                
                AABB extent;
                extent.min = glm::min(v0, v1);
                extent.min = glm::min(extent.min, v2);
                
                extent.max = glm::max(v0, v1);
                extent.max = glm::max(extent.max, v2);
                
                TriangleBox box = {};
                box.vertice_index = i;
                box.extent = extent;
                box.mesh = &shape.mesh;
                
                octree->add(box, box.extent);
            }
        }
    }
};

struct Scene {
    std::vector<std::unique_ptr<Object>> objects;
    
    std::random_device rd;
    std::mt19937 gen;
    std::uniform_real_distribution<> dis;
    
    Scene() : gen(rd()), dis(0.0, 1.0) {}
    
    float distribution() {
        return dis(gen);
    }
    
    Object& load_from_file(const std::string_view path) {
        auto o = std::make_unique<Object>();
        
        tinyobj::LoadObj(&o->attrib, &o->shapes, &o->materials, nullptr, path.data());
      
        return *objects.emplace_back(std::move(o));
    }
    
    void generate_acceleration() {
        for(auto& object : objects) {
            object->create_octree();
        }
    }
};

struct HitResult {
    glm::vec3 position, normal;
    const Object* object = nullptr;
};

std::optional<HitResult> test_mesh(const Ray ray, const Object& object, const tinyobj::mesh_t& mesh, float& tClosest);
std::optional<HitResult> test_scene(const Ray ray, const Scene& scene);
std::optional<HitResult> test_scene_octree(const Ray ray, const Scene& scene);

struct SceneResult {
    HitResult hit;
    glm::vec3 direct, indirect, reflect, combined;
};

std::optional<SceneResult> cast_scene(const Ray ray, Scene& scene, const bool use_bvh, const int depth = 0);

