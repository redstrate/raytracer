#pragma once

#include <optional>
#include <glm/glm.hpp>

#include <tiny_obj_loader.h>

#include "ray.h"
#include "intersections.h"
#include "lighting.h"

constexpr glm::vec3 light_position = glm::vec3(5);
constexpr float light_bias = 0.01f;
constexpr int max_depth = 2;
constexpr int num_indirect_samples = 4;

struct Object {
    glm::vec3 position = glm::vec3(0);
    glm::vec3 color = glm::vec3(1);
    
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
};

struct Scene {
    std::vector<Object> objects;

    Object& load_from_file(const std::string_view path) {
        Object o;
        
        tinyobj::LoadObj(&o.attrib, &o.shapes, &o.materials, nullptr, path.data());
      
        return objects.emplace_back(o);
    }
};

glm::vec3 fetch_position(const Object& object, const tinyobj::mesh_t& mesh, const int32_t index, const int32_t vertex);
glm::vec3 fetch_normal(const Object& object, const tinyobj::mesh_t& mesh, const int32_t index, const int32_t vertex);

struct HitResult {
    glm::vec3 position, normal;
    Object object;
};

std::optional<HitResult> test_mesh(const Ray ray, const Object& object, const tinyobj::mesh_t& mesh, float& tClosest);
std::optional<HitResult> test_scene(const Ray ray, const Scene& scene, float tClosest = std::numeric_limits<float>::infinity());

struct SceneResult {
    HitResult hit;
    glm::vec3 color, indirect;
};

std::optional<SceneResult> cast_scene(const Ray ray, const Scene& scene, const int depth = 0);
