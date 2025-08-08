#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <Mesh.hpp>
#include <Shader.hpp>
#include <Texture.hpp>

class Tangram
{
public:
    Tangram();
    void initialize();
    void update(float deltaTime);
    void render(const std::shared_ptr<Shader>& shader);

private:
    std::vector<std::shared_ptr<Mesh>> pieces;
    std::vector<glm::mat4> transformations;
    void create_pieces();
};

Tangram::Tangram()
{
    initialize();
}

void Tangram::initialize()
{
    create_pieces();
}

void Tangram::create_pieces()
{
    // Large Pink Triangle: (0,0), (0,8), (4,4)
    {
        std::vector<GLfloat> vertices = {
            0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.8f, 0.0f, 0.0f, 1.0f,
            0.4f, 0.4f, 0.0f, 0.5f, 0.5f
        };
        std::vector<unsigned int> indices = {0, 1, 2};
        pieces.push_back(Mesh::create(vertices, indices));
    }

    // Large Blue Triangle: (0,8), (8,8), (4,4)
    {
        std::vector<GLfloat> vertices = {
            0.0f, 0.8f, 0.0f, 0.0f, 1.0f,
            0.8f, 0.8f, 0.0f, 1.0f, 1.0f,
            0.4f, 0.4f, 0.0f, 0.5f, 0.5f
        };
        std::vector<unsigned int> indices = {0, 1, 2};
        pieces.push_back(Mesh::create(vertices, indices));
    }

    // Small Turquoise Triangle: (2,2), (6,2), (4,4)
    {
        std::vector<GLfloat> vertices = {
            0.2f, 0.2f, 0.0f, 0.25f, 0.25f,
            0.6f, 0.2f, 0.0f, 0.75f, 0.25f,
            0.4f, 0.4f, 0.0f, 0.5f, 0.5f
        };
        std::vector<unsigned int> indices = {0, 1, 2};
        pieces.push_back(Mesh::create(vertices, indices));
    }

    // Small Red Triangle: (8,4), (8,8), (6,6)
    {
        std::vector<GLfloat> vertices = {
            0.8f, 0.4f, 0.0f, 1.0f, 0.5f,
            0.8f, 0.8f, 0.0f, 1.0f, 1.0f,
            0.6f, 0.6f, 0.0f, 0.75f, 0.75f
        };
        std::vector<unsigned int> indices = {0, 1, 2};
        pieces.push_back(Mesh::create(vertices, indices));
    }

    // Green Square: (4,4), (6,2), (8,4), (6,6)
    {
        std::vector<GLfloat> vertices = {
            0.4f, 0.4f, 0.0f, 0.5f, 0.5f,
            0.6f, 0.2f, 0.0f, 0.7f, 0.3f,
            0.8f, 0.4f, 0.0f, 0.9f, 0.5f,
            0.6f, 0.6f, 0.0f, 0.7f, 0.7f
        };
        std::vector<unsigned int> indices = {0, 1, 2, 0, 2, 3};
        pieces.push_back(Mesh::create(vertices, indices));
    }

    // Orange Parallelogram: (0,0), (4,0), (6,2), (2,2)
    {
        std::vector<GLfloat> vertices = {
            0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
            0.4f, 0.0f, 0.0f, 0.5f, 0.0f,
            0.6f, 0.2f, 0.0f, 0.7f, 0.3f,
            0.2f, 0.2f, 0.0f, 0.3f, 0.3f
        };
        std::vector<unsigned int> indices = {0, 1, 2, 0, 2, 3};
        pieces.push_back(Mesh::create(vertices, indices));
    }

    // Purple Triangle: (4,0), (8,0), (8,4)
    {
        std::vector<GLfloat> vertices = {
            0.4f, 0.0f, 0.0f, 0.5f, 0.0f,
            0.8f, 0.0f, 0.0f, 0.9f, 0.0f,
            0.8f, 0.4f, 0.0f, 0.9f, 0.5f
        };
        std::vector<unsigned int> indices = {0, 1, 2};
        pieces.push_back(Mesh::create(vertices, indices));
    }

    // Ensure transformations has the same size as pieces
    transformations.resize(pieces.size(), glm::mat4(1.0f));
}

void Tangram::update(float deltaTime)
{
    // Update logic for tangram pieces (e.g., handle user interactions)
}

void Tangram::render(const std::shared_ptr<Shader>& shader)
{
    // Define colors for each piece
    std::vector<glm::vec3> colors = {
        {1.0f, 0.0f, 0.6f}, // Pink
        {0.0f, 0.3f, 1.0f}, // Blue
        {0.0f, 1.0f, 1.0f}, // Turquoise
        {1.0f, 0.0f, 0.0f}, // Red
        {0.0f, 1.0f, 0.0f}, // Green
        {1.0f, 0.5f, 0.0f}, // Orange
        {0.5f, 0.0f, 0.5f}  // Purple
    };

    for (size_t i = 0; i < pieces.size(); ++i)
    {
        glm::mat4 model = transformations[i];
        glUniformMatrix4fv(shader->get_uniform_model_id(), 1, GL_FALSE, glm::value_ptr(model));

        // Set color uniform
        GLint colorLoc = glGetUniformLocation(shader->get_program_id(), "piece_color");
        glUniform3fv(colorLoc, 1, glm::value_ptr(colors[i % colors.size()]));

        pieces[i]->render();
    }
}