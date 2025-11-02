#include <iostream>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <Camera.hpp>
#include <Mesh.hpp>
#include <Shader.hpp>
#include <Window.hpp>
#include <Room.hpp>
#include <PointLight.hpp>
#include <Lightbulb.hpp>

namespace fs = std::filesystem;

struct Data
{
    static std::vector<std::shared_ptr<Shader>> shader_list;
    static const fs::path root_path;
    static const fs::path vertex_shader_path;
    static const fs::path lightbulb_vertex_shader_path;
    static const fs::path fragment_shader_path;
    static const fs::path lightbulb_fragment_shader_path;
};

std::vector<std::shared_ptr<Shader>> Data::shader_list{};
const fs::path Data::root_path{fs::path{__FILE__}.parent_path()};
const fs::path Data::vertex_shader_path{Data::root_path / "shaders" / "shader.vert"};
const fs::path Data::lightbulb_vertex_shader_path{Data::root_path / "shaders" / "lightbulb.vert"};
const fs::path Data::fragment_shader_path{Data::root_path / "shaders" / "shader.frag"};
const fs::path Data::lightbulb_fragment_shader_path{Data::root_path / "shaders" / "lightbulb.frag"};

void create_shaders_program() noexcept
{
    Data::shader_list.push_back(Shader::create_from_files(Data::vertex_shader_path, Data::fragment_shader_path));
    Data::shader_list.push_back(Shader::create_from_files(Data::lightbulb_vertex_shader_path, Data::lightbulb_fragment_shader_path));
}

int main()
{
    // Window dimensions
    constexpr GLint WIDTH = 800;
    constexpr GLint HEIGHT = 600;

    auto main_window = Window::create(WIDTH, HEIGHT, "The Room");

    if (main_window == nullptr){
        return EXIT_FAILURE;
    }

    create_shaders_program();
    
    // Place camera inside the room
    Camera camera{glm::vec3{0.f, 1.f, 0.f}, glm::vec3{0.f, 1.f, 0.f}, -90.f, 0.f, 5.f, 0.15f};

    // Create the room
    Room room(Data::root_path);

    // Create the shared mesh for all lightbulbs
    Lightbulb::create_mesh();

    // Create lightbulbs, which contain the point light data and the mesh
    std::vector<Lightbulb> lightbulbs;
    lightbulbs.emplace_back(
        PointLight(glm::vec3{-4.0f, 7.5f, 0.0f},
                   glm::vec3{0.05f, 0.05f, 0.05f}, glm::vec3{2.0f, 2.0f, 2.0f}, glm::vec3{0.5f, 0.5f, 0.5f},
                   1.0f, 0.09f, 0.032f), // PointLight
        glm::vec3{1.0f, 1.0f, 1.0f});   // Color
    lightbulbs.emplace_back(
        PointLight(glm::vec3{4.0f, 7.5f, 0.0f},
                   glm::vec3{0.05f, 0.05f, 0.05f}, glm::vec3{2.0f, 2.0f, 2.0f}, glm::vec3{0.5f, 0.5f, 0.5f},
                   1.0f, 0.09f, 0.032f), // PointLight
        glm::vec3{1.0f, 1.0f, 1.0f});   // Color

    glm::mat4 projection = glm::perspective(45.f, main_window->get_aspect_ratio(), 0.1f, 100.f);

    GLfloat last_time = glfwGetTime();

    while (!main_window->should_be_closed())
    {
        GLfloat now = glfwGetTime();
        GLfloat dt = now - last_time;
        last_time = now;

        // Get and handle user input events
        glfwPollEvents();

        camera.handle_keys(main_window->get_keys());
        camera.handle_mouse(main_window->get_x_change(), main_window->get_y_change());
        camera.update(dt);

        // Clear the window
        glClearColor(0.f, 0.f, 0.f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // --- Render the room ---
        Data::shader_list[0]->use();

        glUniform1i(Data::shader_list[0]->get_uniform_texture_sampler_id(), 0);
        glUniform1i(glGetUniformLocation(Data::shader_list[0]->get_program_id(), "normal_sampler"), 1);

        // Set projection and view matrices
        glUniformMatrix4fv(Data::shader_list[0]->get_uniform_projection_id(), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(Data::shader_list[0]->get_uniform_view_id(), 1, GL_FALSE, glm::value_ptr(camera.get_view_matrix()));

        // Set lighting and camera position uniforms
        glUniform3fv(glGetUniformLocation(Data::shader_list[0]->get_program_id(), "viewPosition"), 1, glm::value_ptr(camera.get_position()));
        
        // --- Set Light Uniforms ---
        // Directional light
        glUniform3f(glGetUniformLocation(Data::shader_list[0]->get_program_id(), "dirLight.direction"), -0.2f, -1.0f, -0.3f);
        glUniform3f(glGetUniformLocation(Data::shader_list[0]->get_program_id(), "dirLight.diffuse"), 0.5f, 0.5f, 0.5f);
        glUniform3f(glGetUniformLocation(Data::shader_list[0]->get_program_id(), "dirLight.specular"), 0.1f, 0.1f, 0.1f);

        // Point lights
        for(size_t i = 0; i < lightbulbs.size(); ++i) {
            lightbulbs[i].use_light(Data::shader_list[0], i);
        }

        // Render the room with its material properties
        room.render(Data::shader_list[0]);

        // --- Render the lightbulbs ---
        Data::shader_list[1]->use();
        glUniformMatrix4fv(glGetUniformLocation(Data::shader_list[1]->get_program_id(), "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(Data::shader_list[1]->get_program_id(), "view"), 1, GL_FALSE, glm::value_ptr(camera.get_view_matrix()));

        for (const auto& bulb : lightbulbs) {
            bulb.render(Data::shader_list[1]);
        }

        glUseProgram(0);

        main_window->swap_buffers();
    }

    return EXIT_SUCCESS;
}
