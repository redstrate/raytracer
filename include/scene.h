#pragma once

#include <optional>

#include <tiny_obj_loader.h>

struct Scene {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    
    bool load_from_file(const std::string_view path) {
        return tinyobj::LoadObj(&attrib, &shapes, &materials, nullptr, path.data());
    }
};

inline glm::vec3 fetch_position(const Scene& scene, const tinyobj::mesh_t& mesh, const int32_t index, const int32_t vertex) {
    tinyobj::index_t idx = mesh.indices[(index * 3) +vertex];
    
    tinyobj::real_t vx = scene.attrib.vertices[3*idx.vertex_index+0];
    tinyobj::real_t vy = scene.attrib.vertices[3*idx.vertex_index+1];
    tinyobj::real_t vz = scene.attrib.vertices[3*idx.vertex_index+2];
    
    return glm::vec3(vx, vy, vz);
}

inline glm::vec3 fetch_normal(const Scene& scene, const tinyobj::mesh_t& mesh, const int32_t index, const int32_t vertex) {
    tinyobj::index_t idx = mesh.indices[(index * 3) + vertex];
    
    tinyobj::real_t nx = scene.attrib.normals[3*idx.normal_index+0];
    tinyobj::real_t ny = scene.attrib.normals[3*idx.normal_index+1];
    tinyobj::real_t nz = scene.attrib.normals[3*idx.normal_index+2];
    
    return glm::vec3(nx, ny, nz);
}

struct HitResult {
    glm::vec3 position, normal;
    tinyobj::mesh_t* mesh = nullptr;
};


std::optional<HitResult> test_mesh(const Ray ray, const Scene& scene, const tinyobj::mesh_t& mesh, float& tClosest) {
    bool intersection = false;
    HitResult result = {};
    
    for (size_t i = 0; i < mesh.num_face_vertices.size(); i++) {
        const glm::vec3 v0 = fetch_position(scene, mesh, i, 0);
        const glm::vec3 v1 = fetch_position(scene, mesh, i, 1);
        const glm::vec3 v2 = fetch_position(scene, mesh, i, 2);
        
        float t = std::numeric_limits<float>::infinity(), u, v;
        if (intersections::ray_triangle(ray, v0, v1, v2, t, u, v)) {
            if (t < tClosest && t > epsilon) {
                const glm::vec3 n0 = fetch_normal(scene, mesh, i, 0);
                const glm::vec3 n1 = fetch_normal(scene, mesh, i, 1);
                const glm::vec3 n2 = fetch_normal(scene, mesh, i, 2);
                
                result.normal = (1 - u - v) * n0 + u * n1 + v * n2;
                result.position = ray.origin + ray.direction * t;
                
                tClosest = t;
                
                intersection = true;
            }
        }
    }
    
    if(intersection)
        return result;
    else
        return {};
}

std::optional<HitResult> test_scene(const Ray ray, const Scene& scene, float tClosest = std::numeric_limits<float>::infinity()) {
    bool intersection = false;
    HitResult result = {};
    
    for (uint32_t i = 0; i < scene.shapes.size(); i++) {
        auto mesh = scene.shapes[i].mesh;
        
        if(auto hit = test_mesh(ray, scene, mesh, tClosest)) {
            intersection = true;
            result = hit.value();
            result.mesh = &mesh;
        }
    }
    
    if(intersection)
        return result;
    else
        return {};
}
