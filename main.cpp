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

    // Create point lights
    std::vector<PointLight> point_lights;
    point_lights.emplace_back(glm::vec3{-4.0f, 0.0f, 0.0f},  // position
                              glm::vec3{0.1f, 0.1f, 0.1f}, glm::vec3{0.8f, 0.8f, 0.8f}, glm::vec3{1.0f, 1.0f, 1.0f}, // colors
                              1.0f, 0.09f, 0.032f); // attenuation
    point_lights.emplace_back(glm::vec3{4.0f, 0.0f, 0.0f},
                              glm::vec3{0.1f, 0.1f, 0.1f}, glm::vec3{0.8f, 0.8f, 0.8f}, glm::vec3{1.0f, 1.0f, 1.0f},
                              1.0f, 0.09f, 0.032f);

    // Create a cube mesh for the lightbulbs
    std::vector<GLfloat> lightbulb_vertices = {
        -0.1f, -0.1f, -0.1f,
         0.1f, -0.1f, -0.1f,
         0.1f,  0.1f, -0.1f,
        -0.1f,  0.1f, -0.1f,
        -0.1f, -0.1f,  0.1f,
         0.1f, -0.1f,  0.1f,
         0.1f,  0.1f,  0.1f,
        -0.1f,  0.1f,  0.1f,
    };

    std::vector<unsigned int> lightbulb_indices = {
        0, 1, 2, 0, 2, 3, // Back
        4, 5, 6, 4, 6, 7, // Front
        0, 4, 7, 0, 7, 3, // Left
        1, 5, 6, 1, 6, 2, // Right
        3, 2, 6, 3, 6, 7, // Top
        0, 1, 5, 0, 5, 4  // Bottom
    };

    // We need to modify the Mesh class to handle vertices without normals and texture coordinates
    // For now, let's just pad the data. This is inefficient but will work.
    std::vector<GLfloat> padded_lightbulb_vertices;
    for (size_t i = 0; i < lightbulb_vertices.size(); i += 3) {
        padded_lightbulb_vertices.push_back(lightbulb_vertices[i]);
        padded_lightbulb_vertices.push_back(lightbulb_vertices[i+1]);
        padded_lightbulb_vertices.push_back(lightbulb_vertices[i+2]);
        // Pad with 5 zeros for normal and tex coords
        for(int j = 0; j < 5; ++j) padded_lightbulb_vertices.push_back(0.0f);
    }

    auto lightbulb_mesh = Mesh::create(padded_lightbulb_vertices, lightbulb_indices);


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

        // Set projection and view matrices
        glUniformMatrix4fv(Data::shader_list[0]->get_uniform_projection_id(), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(Data::shader_list[0]->get_uniform_view_id(), 1, GL_FALSE, glm::value_ptr(camera.get_view_matrix()));

        // Set lighting and camera position uniforms
        glUniform3fv(glGetUniformLocation(Data::shader_list[0]->get_program_id(), "viewPosition"), 1, glm::value_ptr(camera.get_position()));
        
        // --- Set Light Uniforms ---
        // Directional light
        glUniform3f(glGetUniformLocation(Data::shader_list[0]->get_program_id(), "dirLight.direction"), -0.2f, -1.0f, -0.3f);
        glUniform3f(glGetUniformLocation(Data::shader_list[0]->get_program_id(), "dirLight.diffuse"), 0.4f, 0.4f, 0.4f); // Dimmed a bit
        glUniform3f(glGetUniformLocation(Data::shader_list[0]->get_program_id(), "dirLight.specular"), 0.5f, 0.5f, 0.5f);

        // Point lights
        for(size_t i = 0; i < point_lights.size(); ++i) {
            point_lights[i].use(Data::shader_list[0], i);
        }

        // Render the room with its material properties
        room.render(Data::shader_list[0]);

        // --- Render the lightbulbs ---
        Data::shader_list[1]->use();
        glUniformMatrix4fv(glGetUniformLocation(Data::shader_list[1]->get_program_id(), "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(Data::shader_list[1]->get_program_id(), "view"), 1, GL_FALSE, glm::value_ptr(camera.get_view_matrix()));

        for (const auto& light : point_lights) {
            glm::mat4 model{1.0f};
            // The lightbulb position is stored in the PointLight class, but we can't access it.
            // For now, let's hardcode the positions to match.
            model = glm::translate(model, light.get_position());
            glUniformMatrix4fv(glGetUniformLocation(Data::shader_list[1]->get_program_id(), "model"), 1, GL_FALSE, glm::value_ptr(model));
            glUniform3f(glGetUniformLocation(Data::shader_list[1]->get_program_id(), "lightColor"), 1.0f, 1.0f, 1.0f); // White lightbulbs
            lightbulb_mesh->render();
        }

        glUseProgram(0);

        main_window->swap_buffers();
    }

    return EXIT_SUCCESS;
}
