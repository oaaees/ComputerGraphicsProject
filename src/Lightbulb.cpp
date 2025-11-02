#include <Lightbulb.hpp>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

std::shared_ptr<Mesh> Lightbulb::mesh = nullptr;


Lightbulb::Lightbulb(const PointLight& light, const glm::vec3& color)
    : point_light{light}, bulb_color{color}
{
}

void Lightbulb::render(const std::shared_ptr<Shader>& shader) const
{
    glm::mat4 model{1.0f};
    model = glm::translate(model, point_light.get_position());
    glUniformMatrix4fv(glGetUniformLocation(shader->get_program_id(), "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniform3fv(glGetUniformLocation(shader->get_program_id(), "lightColor"), 1, glm::value_ptr(bulb_color));
    mesh->render();
}

void Lightbulb::use_light(const std::shared_ptr<Shader>& shader, GLuint light_index) const
{
    point_light.use(shader, light_index);
}

void Lightbulb::create_mesh()
{
    std::vector<GLfloat> vertices = {
        -0.1f, -0.1f, -0.1f,
         0.1f, -0.1f, -0.1f,
         0.1f,  0.1f, -0.1f,
        -0.1f,  0.1f, -0.1f,
        -0.1f, -0.1f,  0.1f,
         0.1f, -0.1f,  0.1f,
         0.1f,  0.1f,  0.1f,
        -0.1f,  0.1f,  0.1f,
    };

    std::vector<unsigned int> indices = {
        0, 1, 2, 0, 2, 3, // Back
        4, 5, 6, 4, 6, 7, // Front
        0, 4, 7, 0, 7, 3, // Left
        1, 5, 6, 1, 6, 2, // Right
        3, 2, 6, 3, 6, 7, // Top
        0, 1, 5, 0, 5, 4  // Bottom
    };

    // The main shader expects normals, tex coords, and tangents.
    // The lightbulb shader only uses position, but we use the same Mesh class.
    // So we pad the vertex data to match the expected format.
    std::vector<GLfloat> padded_vertices;
    for (size_t i = 0; i < vertices.size(); i += 3) {
        padded_vertices.insert(padded_vertices.end(), {vertices[i], vertices[i+1], vertices[i+2]});
        // Pad with 8 zeros for normal, tex coords, and tangent
        for(int j = 0; j < 8; ++j) padded_vertices.push_back(0.0f);
    }

    mesh = Mesh::create(padded_vertices, indices);
}