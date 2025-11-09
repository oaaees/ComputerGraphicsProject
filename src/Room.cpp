#include <Room.hpp>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

Room::Room(const std::filesystem::path& root_path)
{
    // Load textures
    floor_texture = std::make_shared<Texture>(root_path / "textures" / "floor_albedo.jpg");
    floor_texture->load();

    floor_normal_texture = std::make_shared<Texture>(root_path / "textures" / "floor_normal.png");
    floor_normal_texture->load();

    wall_texture = std::make_shared<Texture>(root_path / "textures" / "wall_albedo.jpg");
    wall_texture->load();

    wall_normal_texture = std::make_shared<Texture>(root_path / "textures" / "wall_normal.png");
    wall_normal_texture->load();

    // --- Room Geometry ---

    // Floor
    std::vector<GLfloat> floor_vertices = {
        // Positions          // Normals           // Texture Coords
        -10.0f, -2.0f, -10.0f,  0.0f, 1.0f, 0.0f,   0.0f, 0.0f,
         10.0f, -2.0f, -10.0f,  0.0f, 1.0f, 0.0f,   5.0f, 0.0f,
         10.0f, -2.0f,  10.0f,  0.0f, 1.0f, 0.0f,   5.0f, 5.0f,
        -10.0f, -2.0f,  10.0f,  0.0f, 1.0f, 0.0f,   0.0f, 5.0f,
    };
    std::vector<unsigned int> floor_indices = { 0, 2, 1, 0, 3, 2 };
    floor_mesh = Mesh::create(floor_vertices, floor_indices);

    // Ceiling
    std::vector<GLfloat> ceiling_vertices = {
        // Positions          // Normals            // Texture Coords
        -10.0f,  8.0f, -10.0f,  0.0f, -1.0f, 0.0f,   0.0f, 0.0f,
         10.0f,  8.0f, -10.0f,  0.0f, -1.0f, 0.0f,   2.0f, 0.0f,
         10.0f,  8.0f,  10.0f,  0.0f, -1.0f, 0.0f,   2.0f, 1.0f,
        -10.0f,  8.0f,  10.0f,  0.0f, -1.0f, 0.0f,   0.0f, 1.0f,
    };
    std::vector<unsigned int> ceiling_indices = { 0, 1, 2, 0, 2, 3 }; // Reversed winding for downward normal
    ceiling_mesh = Mesh::create(ceiling_vertices, ceiling_indices);

    // Walls
    std::vector<GLfloat> wall_vertices = {
        // Positions          // Normals           // Texture Coords
        // Back wall
        -10.0f, -2.0f, -10.0f,  0.0f, 0.0f, 1.0f,   0.0f, 0.0f,
         10.0f, -2.0f, -10.0f,  0.0f, 0.0f, 1.0f,   2.0f, 0.0f,
         10.0f,  8.0f, -10.0f,  0.0f, 0.0f, 1.0f,   2.0f, 1.0f,
        -10.0f,  8.0f, -10.0f,  0.0f, 0.0f, 1.0f,   0.0f, 1.0f,
        // Front wall
        -10.0f, -2.0f, 10.0f,   0.0f, 0.0f, -1.0f,  2.0f, 0.0f,
         10.0f, -2.0f, 10.0f,   0.0f, 0.0f, -1.0f,  0.0f, 0.0f,
         10.0f,  8.0f, 10.0f,   0.0f, 0.0f, -1.0f,  0.0f, 1.0f,
        -10.0f,  8.0f, 10.0f,   0.0f, 0.0f, -1.0f,  2.0f, 1.0f,
        // Left wall
        -10.0f, -2.0f,  10.0f,  1.0f, 0.0f, 0.0f,   2.0f, 0.0f,
        -10.0f, -2.0f, -10.0f,  1.0f, 0.0f, 0.0f,   0.0f, 0.0f,
        -10.0f,  8.0f, -10.0f,  1.0f, 0.0f, 0.0f,   0.0f, 1.0f,
        -10.0f,  8.0f,  10.0f,  1.0f, 0.0f, 0.0f,   2.0f, 1.0f,
        // Right wall
         10.0f, -2.0f, -10.0f, -1.0f, 0.0f, 0.0f,   2.0f, 0.0f,
         10.0f, -2.0f,  10.0f, -1.0f, 0.0f, 0.0f,   0.0f, 0.0f,
         10.0f,  8.0f,  10.0f, -1.0f, 0.0f, 0.0f,   0.0f, 1.0f,
         10.0f,  8.0f, -10.0f, -1.0f, 0.0f, 0.0f,   2.0f, 1.0f,
    };
    std::vector<unsigned int> wall_indices = {
        0, 1, 2, 0, 2, 3,       // Back
        4, 5, 6, 4, 6, 7,       // Front
        8, 9, 10, 8, 10, 11,    // Left
        12, 13, 14, 12, 14, 15  // Right
    };
    wall_mesh = Mesh::create(wall_vertices, wall_indices);
}

void Room::render(const std::shared_ptr<Shader>& shader)
{
    glm::mat4 model{1.f};
    glUniformMatrix4fv(shader->get_uniform_model_id(), 1, GL_FALSE, glm::value_ptr(model));

    // Render floor (more shiny)
    glUniform1f(glGetUniformLocation(shader->get_program_id(), "material.shininess"), 32.0f);
    floor_texture->use(); // Binds to GL_TEXTURE0
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, floor_normal_texture->get_id());
    floor_mesh->render();

    // Render ceiling (less shiny)
    glUniform1f(glGetUniformLocation(shader->get_program_id(), "material.shininess"), 16.0f);
    wall_texture->use(); // Use the wall's albedo texture
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, wall_normal_texture->get_id()); // Use the wall's normal map
    ceiling_mesh->render();

    // Render walls (matte)
    glUniform1f(glGetUniformLocation(shader->get_program_id(), "material.shininess"), 64.0f);
    wall_texture->use(); // Binds to GL_TEXTURE0
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, wall_normal_texture->get_id());
    wall_mesh->render();
}