<<<<<<< HEAD
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <Shader.hpp>
<<<<<<< HEAD
#include <Tangram.hpp>

Tangram::Tangram()
{
    initialize();
}

void Tangram::initialize()
{
=======
#include <Texture.hpp>

=======
>>>>>>> c176f33 (pieces can move and rotate)
#include <Tangram.hpp>

Tangram::Tangram(){
>>>>>>> 6e700d4 (just shows tangram)
    create_pieces();
}

void Tangram::create_pieces()
{
    // Large Triangle: (0,0), (0,8), (4,4)
    {
        std::vector<GLfloat> vertices = {
            0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.8f, 0.0f, 0.0f, 1.0f,
            0.4f, 0.4f, 0.0f, 0.5f, 0.5f};
        std::vector<unsigned int> indices = {0, 1, 2};
        pieces.push_back(Mesh::create(vertices, indices));
    }

    // Large Triangle 2: (0,8), (8,8), (4,4)
    {
        std::vector<GLfloat> vertices = {
            0.0f, 0.8f, 0.0f, 0.0f, 1.0f,
            0.8f, 0.8f, 0.0f, 1.0f, 1.0f,
            0.4f, 0.4f, 0.0f, 0.5f, 0.5f};
        std::vector<unsigned int> indices = {0, 1, 2};
        pieces.push_back(Mesh::create(vertices, indices));
    }

    // Small Triangle: (2,2), (6,2), (4,4)
    {
        std::vector<GLfloat> vertices = {
            0.2f, 0.2f, 0.0f, 0.25f, 0.25f,
            0.6f, 0.2f, 0.0f, 0.75f, 0.25f,
            0.4f, 0.4f, 0.0f, 0.5f, 0.5f};
        std::vector<unsigned int> indices = {0, 1, 2};
        pieces.push_back(Mesh::create(vertices, indices));
    }

    // Small Triangle: (8,4), (8,8), (6,6)
    {
        std::vector<GLfloat> vertices = {
            0.8f, 0.4f, 0.0f, 1.0f, 0.5f,
            0.8f, 0.8f, 0.0f, 1.0f, 1.0f,
            0.6f, 0.6f, 0.0f, 0.75f, 0.75f};
        std::vector<unsigned int> indices = {0, 1, 2};
        pieces.push_back(Mesh::create(vertices, indices));
    }

    // Square: (4,4), (6,2), (8,4), (6,6)
    {
        std::vector<GLfloat> vertices = {
            0.4f, 0.4f, 0.0f, 0.5f, 0.5f,
            0.6f, 0.2f, 0.0f, 0.7f, 0.3f,
            0.8f, 0.4f, 0.0f, 0.9f, 0.5f,
            0.6f, 0.6f, 0.0f, 0.7f, 0.7f};
        std::vector<unsigned int> indices = {0, 1, 2, 0, 2, 3};
        pieces.push_back(Mesh::create(vertices, indices));
    }

    // Parallelogram: (0,0), (4,0), (6,2), (2,2)
    {
        std::vector<GLfloat> vertices = {
            0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
            0.4f, 0.0f, 0.0f, 0.5f, 0.0f,
            0.6f, 0.2f, 0.0f, 0.7f, 0.3f,
            0.2f, 0.2f, 0.0f, 0.3f, 0.3f};
        std::vector<unsigned int> indices = {0, 1, 2, 0, 2, 3};
        pieces.push_back(Mesh::create(vertices, indices));
    }

    // Triangle: (4,0), (8,0), (8,4)
    {
        std::vector<GLfloat> vertices = {
            0.4f, 0.0f, 0.0f, 0.5f, 0.0f,
            0.8f, 0.0f, 0.0f, 0.9f, 0.0f,
            0.8f, 0.4f, 0.0f, 0.9f, 0.5f};
        std::vector<unsigned int> indices = {0, 1, 2};
        pieces.push_back(Mesh::create(vertices, indices));
    }

    // Ensure transformations has the same size as pieces
    transformations.resize(pieces.size(), glm::mat4(1.0f));
}

<<<<<<< HEAD
void Tangram::select_piece(int index)
{
    if (index >= 0 && index < static_cast<int>(pieces.size()))
    {
        selected_piece = index;
    }
}

void Tangram::deselect_piece()
{
    selected_piece = -1;
}

void Tangram::update(float deltaTime, const std::array<bool, 1024> &keys)
{
    if (selected_piece != -1)
    {
        // glm::mat4 &transform = transformations[selected_piece];
        float moveSpeed = 1.0f * deltaTime;

        glm::mat4 &transform = transformations[selected_piece];

        if (keys[GLFW_KEY_W])
            transform = glm::translate(transform, glm::vec3(0.f, moveSpeed, 0.f));
        if (keys[GLFW_KEY_S])
            transform = glm::translate(transform, glm::vec3(0.f, -moveSpeed, 0.f));
        if (keys[GLFW_KEY_D])
            transform = glm::translate(transform, glm::vec3(-moveSpeed, 0.f, 0.f));
        if (keys[GLFW_KEY_A])
            transform = glm::translate(transform, glm::vec3(moveSpeed, 0.f, 0.f));
=======
void Tangram::handle_keys(const std::array<bool, 1024>& keys){
    if (keys[GLFW_KEY_TAB]) {
        selected_piece = (selected_piece + 1) % pieces.size();
    }

    glm::vec3 translation(0.0f);
    if (keys[GLFW_KEY_W]) translation.y += move_step;
    if (keys[GLFW_KEY_S]) translation.y -= move_step;
    if (keys[GLFW_KEY_A]) translation.x -= move_step;
    if (keys[GLFW_KEY_D]) translation.x += move_step;

    if (translation.x != 0.0f || translation.y != 0.0f) {
        transformations[selected_piece] = glm::translate(transformations[selected_piece], translation);
    }

    if (keys[GLFW_KEY_Q]) {
        transformations[selected_piece] = glm::rotate(transformations[selected_piece], rotate_step, glm::vec3(0, 0, 1));
    }
    if (keys[GLFW_KEY_E]) {
        transformations[selected_piece] = glm::rotate(transformations[selected_piece], -rotate_step, glm::vec3(0, 0, 1));
>>>>>>> c176f33 (pieces can move and rotate)
    }
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

    for (size_t i = 0; i < pieces.size(); ++i){
        glm::mat4 model = transformations[i];
        glUniformMatrix4fv(shader->get_uniform_model_id(), 1, GL_FALSE, glm::value_ptr(model));

        // Set color uniform
        GLint colorLoc = glGetUniformLocation(shader->get_program_id(), "piece_color");
        glUniform3fv(colorLoc, 1, glm::value_ptr(colors[i]));

        pieces[i]->render();
    }
}