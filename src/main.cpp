#include <iostream>
#include <limits>
#include <future>
#include <vector>
#include <glm/glm.hpp>
#include <SDL.h>
#include <array>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include "intersections.h"
#include "camera.h"
#include "image.h"
#include "lighting.h"
#include "scene.h"
#include "glad/glad.h"
#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

// scene information
constexpr int32_t width = 512, height = 512;
constexpr glm::vec3 light_position = glm::vec3(5);
const Camera camera = [] {
    Camera camera;
    camera.look_at(glm::vec3(0, 0, 4), glm::vec3(0));
    
    return camera;
}();
constexpr glm::vec3 model_color = glm::vec3(1.0f, 1.0f, 1.0f);

// internal variables
constexpr float light_bias = 0.01f;
constexpr int32_t tile_size = 32;
constexpr int32_t num_tiles_x = width / tile_size;
constexpr int32_t num_tiles_y = height / tile_size;

// globals
Scene scene = {};
Image<glm::vec4, width, height> colors = {};

bool calculate_tile(const int32_t from_x, const int32_t to_width, const int32_t from_y, const int32_t to_height) {
    for (int32_t y = from_y; y < (from_y + to_height); y++) {
        for (int32_t x = from_x; x < (from_x + to_width); x++) {
            Ray ray_camera = camera.get_ray(x, y, width, height);
            
            if (auto hit = test_scene(ray_camera, scene)) {
                const float diffuse = lighting::point_light(hit->position, light_position, hit->normal);
                
                //shadow calculation
                float shadow = 0.0f;
                if(glm::dot(light_position - hit->position, hit->normal) > 0) {
                    const glm::vec3 light_dir = glm::normalize(light_position - hit->position);
                    
                    const Ray shadow_ray(hit->position + (hit->normal * light_bias), light_dir);
                    
                    if(test_scene(shadow_ray, scene))
                        shadow = 1.0f;
                }
                
                const glm::vec3 finalColor = model_color * diffuse * (1.0f - shadow);
                colors.get(x, y) = glm::vec4(finalColor, 1.0f);
            }
        }
    }
    
    return true;
}

GLuint quad_vao = 0;
GLuint pixel_program = 0;
GLuint pixels_texture = 0;

void setup_gfx() {
    // create quad for pixel rendering
    constexpr std::array vertices = {
        -0.5f,  0.5f, 0.0f, 0.0f, 0.0f,
        0.5f,  0.5f, 0.0f, 1.0f, 0.0f,
        0.5f, -0.5f, 0.0f, 1.0f, -1.0f,
        -0.5f, -0.5f, 0.0f, 0.0f, -1.0f // we render it upside down (to opengl) so we flip our tex coord
    };
    
    constexpr std::array elements = {
        0, 1, 2,
        2, 3, 0
    };
    
    glGenVertexArrays(1, &quad_vao);
    glBindVertexArray(quad_vao);
    
    GLuint vbo = 0;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
    
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    
    GLuint ebo = 0;
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, elements.size() * sizeof(uint32_t), elements.data(), GL_STATIC_DRAW);
    
    constexpr std::string_view vertex_glsl =
        "#version 330 core\n"
        "layout (location = 0) in vec3 in_position;\n"
        "layout (location = 1) in vec2 in_uv;\n"
        "out vec2 uv;\n"
        "void main()\n"
        "{\n"
        "   gl_Position = vec4(in_position.xyz, 1.0);\n"
        "   uv = in_uv;\n"
        "}\n";
    const char* vertex_src = vertex_glsl.data();
    
    constexpr std::string_view fragment_glsl =
        "#version 330 core\n"
        "in vec2 uv;\n"
        "out vec4 out_color;\n"
        "uniform sampler2D pixel_texture;\n"
        "void main()\n"
        "{\n"
        "    out_color = texture(pixel_texture, uv);\n"
        "}\n";
    const char* fragment_src = fragment_glsl.data();
    
    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_src, nullptr);
    glCompileShader(vertex_shader);
    
    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_src, nullptr);
    glCompileShader(fragment_shader);
    
    pixel_program = glCreateProgram();
    
    glAttachShader(pixel_program, vertex_shader);
    glAttachShader(pixel_program, fragment_shader);
    glLinkProgram(pixel_program);
    
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    
    glGenTextures(1, &pixels_texture);
    glBindTexture(GL_TEXTURE_2D, pixels_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void update_texture() {
    glBindTexture(GL_TEXTURE_2D, pixels_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_FLOAT, colors.array);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void render() {
    std::vector<std::future<bool>> futures;
    for(int32_t y = 0; y < num_tiles_y; y++)
        for(int32_t x = 0; x < num_tiles_x; x++)
            futures.push_back(std::async(std::launch::async, calculate_tile, x * tile_size, tile_size, y * tile_size, tile_size));
    
    for(auto& future : futures)
        future.wait();
    
    update_texture();
}

void dump_to_file() {
    uint8_t pixels[width * height * 3] = {};
    int i = 0;
    for (int32_t y = height - 1; y >= 0; y--) {
        for (int32_t x = 0; x < width; x++) {
            const glm::ivec4 c = colors.get(x, y);
            pixels[i++] = c.r;
            pixels[i++] = c.g;
            pixels[i++] = c.b;
        }
    }

    stbi_write_png("output.png", width, height, 3, pixels, width * 3);
}

int main(int, char*[]) {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);
    
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    
    SDL_Window* window = SDL_CreateWindow("raytracer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
        
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1);
    
    gladLoadGL();
    setup_gfx();
    
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init("#version 330 core");
    
    bool running = true;
    while(running) {
        SDL_Event event = {};
        while(SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);

            if(event.type == SDL_QUIT)
                running = false;
        }
        
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame(window);
                
        ImGui::NewFrame();
        
        if(ImGui::BeginMainMenuBar()) {
            if(ImGui::BeginMenu("File")) {
                if(ImGui::Button("Load"))
                    scene.load_from_file("suzanne.obj");
                
                ImGui::EndMenu();
            }
            
            ImGui::EndMainMenuBar();
        }
        
        if(ImGui::Button("Render"))
            render();
        
        if(ImGui::Button("Dump to file"))
            dump_to_file();
        
        ImGui::Render();
        
        auto& io = ImGui::GetIO();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClear(GL_COLOR_BUFFER_BIT);
        
        glUseProgram(pixel_program);
        glBindVertexArray(quad_vao);
        glBindTexture(GL_TEXTURE_2D, pixels_texture);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        
        SDL_GL_SwapWindow(window);
    }

    return 0;
}
