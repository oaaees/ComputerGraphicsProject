#include <Room.hpp>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

Room::Room(const std::filesystem::path& root_path)
{
    // Load textures
    floor_texture = std::make_shared<Texture>(root_path / "textures" / "wood.png");
    floor_texture->load();

    wall_texture = std::make_shared<Texture>(root_path / "textures" / "brick.png");
    wall_texture->load();

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

    // Walls
    std::vector<GLfloat> wall_vertices = {
        // Positions          // Normals           // Texture Coords
        // Back wall
        -10.0f, -2.0f, -10.0f,  0.0f, 0.0f, 1.0f,   0.0f, 0.0f,
         10.0f, -2.0f, -10.0f,  0.0f, 0.0f, 1.0f,   5.0f, 0.0f,
         10.0f,  8.0f, -10.0f,  0.0f, 0.0f, 1.0f,   5.0f, 5.0f,
        -10.0f,  8.0f, -10.0f,  0.0f, 0.0f, 1.0f,   0.0f, 5.0f,
        // Front wall
        -10.0f, -2.0f, 10.0f,   0.0f, 0.0f, -1.0f,  5.0f, 0.0f,
         10.0f, -2.0f, 10.0f,   0.0f, 0.0f, -1.0f,  0.0f, 0.0f,
         10.0f,  8.0f, 10.0f,   0.0f, 0.0f, -1.0f,  0.0f, 5.0f,
        -10.0f,  8.0f, 10.0f,   0.0f, 0.0f, -1.0f,  5.0f, 5.0f,
        // Left wall
        -10.0f, -2.0f,  10.0f,  1.0f, 0.0f, 0.0f,   5.0f, 0.0f,
        -10.0f, -2.0f, -10.0f,  1.0f, 0.0f, 0.0f,   0.0f, 0.0f,
        -10.0f,  8.0f, -10.0f,  1.0f, 0.0f, 0.0f,   0.0f, 5.0f,
        -10.0f,  8.0f,  10.0f,  1.0f, 0.0f, 0.0f,   5.0f, 5.0f,
        // Right wall
         10.0f, -2.0f, -10.0f, -1.0f, 0.0f, 0.0f,   5.0f, 0.0f,
         10.0f, -2.0f,  10.0f, -1.0f, 0.0f, 0.0f,   0.0f, 0.0f,
         10.0f,  8.0f,  10.0f, -1.0f, 0.0f, 0.0f,   0.0f, 5.0f,
         10.0f,  8.0f, -10.0f, -1.0f, 0.0f, 0.0f,   5.0f, 5.0f,
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

    floor_texture->use();
    floor_mesh->render();

    wall_texture->use();
    wall_mesh->render();
}