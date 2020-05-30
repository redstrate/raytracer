#pragma once

#include <optional>

#include <tiny_obj_loader.h>

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

inline glm::vec3 fetch_position(const Object& object, const tinyobj::mesh_t& mesh, const int32_t index, const int32_t vertex) {
    const tinyobj::index_t idx = mesh.indices[(index * 3) +vertex];
    
    const auto vx = object.attrib.vertices[3*idx.vertex_index+0];
    const auto vy = object.attrib.vertices[3*idx.vertex_index+1];
    const auto vz = object.attrib.vertices[3*idx.vertex_index+2];
    
    return glm::vec3(vx, vy, vz);
}

inline glm::vec3 fetch_normal(const Object& object, const tinyobj::mesh_t& mesh, const int32_t index, const int32_t vertex) {
    const tinyobj::index_t idx = mesh.indices[(index * 3) + vertex];
    
    const auto nx = object.attrib.normals[3*idx.normal_index+0];
    const auto ny = object.attrib.normals[3*idx.normal_index+1];
    const auto nz = object.attrib.normals[3*idx.normal_index+2];
    
    return glm::vec3(nx, ny, nz);
}

struct HitResult {
    glm::vec3 position, normal;
    Object object;
};

std::optional<HitResult> test_mesh(const Ray ray, const Object& object, const tinyobj::mesh_t& mesh, float& tClosest) {
    bool intersection = false;
    HitResult result = {};
    
    for(size_t i = 0; i < mesh.num_face_vertices.size(); i++) {
        const glm::vec3 v0 = fetch_position(object, mesh, i, 0) + object.position;
        const glm::vec3 v1 = fetch_position(object, mesh, i, 1) + object.position;
        const glm::vec3 v2 = fetch_position(object, mesh, i, 2) + object.position;
        
        float t = std::numeric_limits<float>::infinity(), u, v;
        if(intersections::ray_triangle(ray, v0, v1, v2, t, u, v)) {
            if(t < tClosest && t > epsilon) {
                const glm::vec3 n0 = fetch_normal(object, mesh, i, 0);
                const glm::vec3 n1 = fetch_normal(object, mesh, i, 1);
                const glm::vec3 n2 = fetch_normal(object, mesh, i, 2);
                
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
    
    for(auto& object : scene.objects) {
        for(uint32_t i = 0; i < object.shapes.size(); i++) {
            auto mesh = object.shapes[i].mesh;
            
            if(const auto hit = test_mesh(ray, object, mesh, tClosest)) {
                intersection = true;
                result = hit.value();
                result.object = object;
            }
        }
    }

    if(intersection)
        return result;
    else
        return {};
}

constexpr glm::vec3 light_position = glm::vec3(5);
constexpr float light_bias = 0.01f;
constexpr int max_depth = 2;
constexpr int num_indirect_samples = 4;

struct SceneResult {
    HitResult hit;
    glm::vec3 color, indirect;
};

// methods adapated from https://users.cg.tuwien.ac.at/zsolnai/gfx/smallpaint/
inline std::tuple<glm::vec3, glm::vec3> orthogonal_system(const glm::vec3& v1) {
    glm::vec3 v2;
    if(glm::abs(v1.x) > glm::abs(v1.y)) {
        // project to the y = 0 plane and construct a normalized orthogonal vector in this plane
        const float inverse_length = 1.0f / sqrtf(v1.x * v1.x + v1.z * v1.z);
        v2 = glm::vec3(-v1.z * inverse_length, 0.0f, v1.x * inverse_length);
    } else {
        // project to the x = 0 plane and construct a normalized orthogonal vector in this plane
        const float inverse_length = 1.0f / sqrtf(v1.y * v1.y + v1.z * v1.z);
        v2 = glm::vec3(0.0f, v1.z * inverse_length, -v1.y * inverse_length);
    }
    
    return {v2, glm::cross(v1, v2)};
}

glm::vec3 hemisphere(const double u1, const double u2) {
    const double r = sqrt(1.0 - u1 * u1);
    const double phi = 2 * M_PI * u2;
    
    return glm::vec3(cos(phi) * r, sin(phi) * r, u1);
}

std::optional<SceneResult> cast_scene(const Ray ray, const Scene& scene, const int depth = 0) {
    if(depth > max_depth)
        return {};
    
    if(auto hit = test_scene(ray, scene)) {
        const float diffuse = lighting::point_light(hit->position, light_position, hit->normal);
        
        //shadow calculation
        glm::vec3 direct(0);
        if(glm::dot(light_position - hit->position, hit->normal) > 0) {
            const glm::vec3 light_dir = glm::normalize(light_position - hit->position);
            
            const Ray shadow_ray(hit->position + (hit->normal * light_bias), light_dir);
            
            const float shadow = test_scene(shadow_ray, scene) ? 0.0f : 1.0f;
            
            direct = diffuse * shadow * glm::vec3(1);
        }
        
        glm::vec3 indirect(0);
        for(int i = 0; i < num_indirect_samples; i++) {
            const float theta = drand48() * M_PI;
            const float cos_theta = cos(theta);
            const float sin_theta = sin(theta);
            
            const auto [rotX, rotY] = orthogonal_system(hit->normal);
            
            const glm::vec3 sampled_dir = hemisphere(cos_theta, sin_theta);
            const glm::vec3 rotated_dir = {
                glm::dot({rotX.x, rotY.x, hit->normal.x}, sampled_dir),
                glm::dot({rotX.y, rotY.y, hit->normal.y}, sampled_dir),
                glm::dot({rotX.z, rotY.z, hit->normal.z}, sampled_dir)
            };
            
            if(const auto indirect_result = cast_scene(Ray(ray.origin, rotated_dir), scene, depth + 1))
                indirect += indirect_result->color * cos_theta;
        }
        indirect /= num_indirect_samples;
        
        SceneResult result = {};
        result.hit = *hit;
        result.color = (indirect + direct) * hit->object.color;
        result.indirect = indirect;
        
        return result;
    } else {
        return {};
    }
}
