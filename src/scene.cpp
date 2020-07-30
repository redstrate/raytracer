#include "scene.h"

glm::vec3 fetch_position(const Object& object, const tinyobj::mesh_t& mesh, const int32_t index, const int32_t vertex) {
    const tinyobj::index_t idx = mesh.indices[(index * 3) +vertex];
    
    const auto vx = object.attrib.vertices[3*idx.vertex_index+0];
    const auto vy = object.attrib.vertices[3*idx.vertex_index+1];
    const auto vz = object.attrib.vertices[3*idx.vertex_index+2];
    
    return glm::vec3(vx, vy, vz);
}

glm::vec3 fetch_normal(const Object& object, const tinyobj::mesh_t& mesh, const int32_t index, const int32_t vertex) {
    const tinyobj::index_t idx = mesh.indices[(index * 3) + vertex];
    
    const auto nx = object.attrib.normals[3*idx.normal_index+0];
    const auto ny = object.attrib.normals[3*idx.normal_index+1];
    const auto nz = object.attrib.normals[3*idx.normal_index+2];
    
    return glm::vec3(nx, ny, nz);
}

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

std::optional<HitResult> test_triangle(const Ray ray, const Object& object, const tinyobj::mesh_t& mesh, const size_t i, float& tClosest) {
    bool intersection = false;
    HitResult result = {};
    
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
    
    if(intersection)
        return result;
    else
        return {};
}

std::optional<HitResult> test_scene(const Ray ray, const Scene& scene, float tClosest) {
    bool intersection = false;
    HitResult result = {};
    
    for(auto& object : scene.objects) {
        for(uint32_t i = 0; i < object->shapes.size(); i++) {
            auto mesh = object->shapes[i].mesh;
            
            if(const auto hit = test_mesh(ray, *object, mesh, tClosest)) {
                intersection = true;
                result = hit.value();
                result.object = object.get();
            }
        }
    }
    
    if(intersection)
        return result;
    else
        return {};
}

template<typename T>
Node<T>* find_hit_ray_search(Node<T>& node, const Ray ray, std::vector<Node<T>*>* out) {
    if(node.extent.contains(ray)) {
        if(node.is_split) {
            for(auto& child : node.children)
                find_hit_ray_search(*child, ray, out);
        } else {
            out->push_back(&node);
        }
    }
}

template<typename T>
std::vector<Node<T>*> find_hit_ray(Node<T>& node, const Ray ray) {
    std::vector<Node<T>*> vec;
    
    find_hit_ray_search(node, ray, &vec);
    
    return vec;
}

std::optional<HitResult> test_scene_octree(const Ray ray, const Scene& scene, float tClosest) {
    bool intersection = false;
    HitResult result = {};
    
    for(auto& object : scene.objects) {
        for(auto& node : find_hit_ray(object->octree->root, ray)) {
            for(auto& triangle_object : node->contained_objects) {
                if(const auto hit = test_triangle(ray, *object, *triangle_object.mesh, triangle_object.vertice_index, tClosest)) {
                    intersection = true;
                    result = hit.value();
                    result.object = object.get();
                }
            }
        }
    }
    
    if(intersection)
        return result;
    else
        return {};
}

// methods adapated from https://users.cg.tuwien.ac.at/zsolnai/gfx/smallpaint/
std::tuple<glm::vec3, glm::vec3> orthogonal_system(const glm::vec3& v1) {
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

std::optional<SceneResult> cast_scene(const Ray ray, const Scene& scene, const int depth) {
    if(depth > max_depth)
        return {};
    
    if(auto hit = test_scene_octree(ray, scene)) {
        const float diffuse = lighting::point_light(hit->position, light_position, hit->normal);
        
        // direct lighting calculation
        // currently only supports only one light (directional)
        glm::vec3 direct(0);
        if(glm::dot(light_position - hit->position, hit->normal) > 0) {
            const glm::vec3 light_dir = glm::normalize(light_position - hit->position);
            
            const Ray shadow_ray(hit->position + (hit->normal * light_bias), light_dir);
            
            const float shadow = test_scene_octree(shadow_ray, scene) ? 0.0f : 1.0f;
            
            direct = diffuse * shadow * glm::vec3(1);
        }
        
        // indirect lighting calculation
        // we take a hemisphere orthogonal to the normal, and take a constant number of num_indirect_samples
        // and naive monte carlo without PDF
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
        result.color = (indirect + direct) * hit->object->color;
        result.indirect = indirect;
        
        return result;
    } else {
        return {};
    }
}
