#include <Tangram.hpp>

Tangram::Tangram(){
    create_pieces();
}

void Tangram::create_pieces(){
    float depth = 0.2f;

    // Large Triangle: (0,0), (0,8), (4,4)
    {
        std::vector<GLfloat> vertices = {
            0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.8f, 0.0f, 0.0f, 1.0f,
            0.4f, 0.4f, 0.0f, 0.5f, 0.5f,

            0.0f, 0.0f, depth, 0.0f, 0.0f,
            0.0f, 0.8f, depth, 0.0f, 1.0f,
            0.4f, 0.4f, depth, 0.5f, 0.5f
        };

        std::vector<unsigned int> indices = {0, 1, 2,           // Front face
                                             3, 4, 5,           // Back face
                                             0,1,4,0,4,3,       // Side 1
                                             1,2,5,1,5,4,       // Side 2
                                             2,0,3,2,3,5 };     // Side 3

        pieces.push_back(Mesh::create(vertices, indices));
        piece_centers.push_back(calc_piece_center(vertices));
    }

    // Large Triangle 2: (0,8), (8,8), (4,4)
    {
        std::vector<GLfloat> vertices = {
            0.0f, 0.8f, 0.0f, 0.0f, 1.0f,
            0.8f, 0.8f, 0.0f, 1.0f, 1.0f,
            0.4f, 0.4f, 0.0f, 0.5f, 0.5f,

            0.0f, 0.8f, depth, 0.0f, 1.0f,
            0.8f, 0.8f, depth, 1.0f, 1.0f,
            0.4f, 0.4f, depth, 0.5f, 0.5f
        };

        std::vector<unsigned int> indices = {0, 1, 2,           // Front face
                                             3, 4, 5,           // Back face
                                             0,1,4,0,4,3,       // Side 1
                                             1,2,5,1,5,4,       // Side 2
                                             2,0,3,2,3,5 };     // Side 3

        pieces.push_back(Mesh::create(vertices, indices));
        piece_centers.push_back(calc_piece_center(vertices));
    }

    // Small Triangle: (2,2), (6,2), (4,4)
    {
        std::vector<GLfloat> vertices = {
            0.2f, 0.2f, 0.0f, 0.25f, 0.25f,
            0.6f, 0.2f, 0.0f, 0.75f, 0.25f,
            0.4f, 0.4f, 0.0f, 0.5f, 0.5f,

            0.2f, 0.2f, depth, 0.25f, 0.25f,
            0.6f, 0.2f, depth, 0.75f, 0.25f,
            0.4f, 0.4f, depth, 0.5f, 0.5f
        };

        std::vector<unsigned int> indices = {0, 1, 2,           // Front face
                                             3, 4, 5,           // Back face
                                             0,1,4,0,4,3,       // Side 1
                                             1,2,5,1,5,4,       // Side 2
                                             2,0,3,2,3,5 };     // Side 3

        pieces.push_back(Mesh::create(vertices, indices));
        piece_centers.push_back(calc_piece_center(vertices));
    }

    // Small Triangle: (8,4), (8,8), (6,6)
    {
        std::vector<GLfloat> vertices = {
            0.8f, 0.4f, 0.0f, 1.0f, 0.5f,
            0.8f, 0.8f, 0.0f, 1.0f, 1.0f,
            0.6f, 0.6f, 0.0f, 0.75f, 0.75f,

            0.8f, 0.4f, depth, 1.0f, 0.5f,
            0.8f, 0.8f, depth, 1.0f, 1.0f,
            0.6f, 0.6f, depth, 0.75f, 0.75f
        };

        std::vector<unsigned int> indices = {0, 1, 2,           // Front face
                                             3, 4, 5,           // Back face
                                             0,1,4,0,4,3,       // Side 1
                                             1,2,5,1,5,4,       // Side 2
                                             2,0,3,2,3,5 };     // Side 3
                                        
        pieces.push_back(Mesh::create(vertices, indices));
        piece_centers.push_back(calc_piece_center(vertices));
    }

    // Square: (4,4), (6,2), (8,4), (6,6)
    {
        std::vector<GLfloat> vertices = {
            0.4f, 0.4f, 0.0f, 0.5f, 0.5f,
            0.6f, 0.2f, 0.0f, 0.7f, 0.3f,
            0.8f, 0.4f, 0.0f, 0.9f, 0.5f,
            0.6f, 0.6f, 0.0f, 0.7f, 0.7f,

            0.4f, 0.4f, depth, 0.5f, 0.5f,
            0.6f, 0.2f, depth, 0.7f, 0.3f,
            0.8f, 0.4f, depth, 0.9f, 0.5f,
            0.6f, 0.6f, depth, 0.7f, 0.7f
        };

        std::vector<unsigned int> indices = {0, 1, 2, 0, 2, 3,      // Front Face
                                             4, 5, 6, 4, 6, 7,      // Back Face
                                             0, 1, 5, 0, 5, 4,      // Sides
                                             1, 2, 6, 1, 6, 5,
                                             2, 3, 7, 2, 7, 6,
                                             3, 0, 4, 3, 4, 7};
        pieces.push_back(Mesh::create(vertices, indices));
        piece_centers.push_back(calc_piece_center(vertices));
    }

    // Parallelogram: (0,0), (4,0), (6,2), (2,2)
    {
        std::vector<GLfloat> vertices = {
            0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
            0.4f, 0.0f, 0.0f, 0.5f, 0.0f,
            0.6f, 0.2f, 0.0f, 0.7f, 0.3f,
            0.2f, 0.2f, 0.0f, 0.3f, 0.3f,

            0.0f, 0.0f, depth, 0.0f, 0.0f,
            0.4f, 0.0f, depth, 0.5f, 0.0f,
            0.6f, 0.2f, depth, 0.7f, 0.3f,
            0.2f, 0.2f, depth, 0.3f, 0.3f
        };

        std::vector<unsigned int> indices = {0, 1, 2, 0, 2, 3,      // Front Face
                                             4, 5, 6, 4, 6, 7,      // Back Face
                                             0, 1, 5, 0, 5, 4,      // Sides
                                             1, 2, 6, 1, 6, 5,
                                             2, 3, 7, 2, 7, 6,
                                             3, 0, 4, 3, 4, 7};

        pieces.push_back(Mesh::create(vertices, indices));
        piece_centers.push_back(calc_piece_center(vertices));
    }

    // Triangle: (4,0), (8,0), (8,4)
    {
        std::vector<GLfloat> vertices = {
            0.4f, 0.0f, 0.0f, 0.5f, 0.0f,
            0.8f, 0.0f, 0.0f, 0.9f, 0.0f,
            0.8f, 0.4f, 0.0f, 0.9f, 0.5f,

            0.4f, 0.0f, depth, 0.5f, 0.0f,
            0.8f, 0.0f, depth, 0.9f, 0.0f,
            0.8f, 0.4f, depth, 0.9f, 0.5f
        };
        std::vector<unsigned int> indices = {0, 1, 2,           // Front face
                                             3, 4, 5,           // Back face
                                             0,1,4,0,4,3,       // Side 1
                                             1,2,5,1,5,4,       // Side 2
                                             2,0,3,2,3,5 };     // Side 3

        pieces.push_back(Mesh::create(vertices, indices));
        piece_centers.push_back(calc_piece_center(vertices));
    }


    // Ensure transformations has the same size as pieces
    transformations.resize(pieces.size(), glm::mat4(1.0f));
    piece_positions.resize(pieces.size(), glm::vec2(0.0f));
    piece_rotations.resize(pieces.size(), 0.0f);
}

glm::vec2 Tangram::calc_piece_center(std::vector<GLfloat> vertices){
    float cx = 0.0f, cy = 0.0f;
    int num_verts = vertices.size() / 5;

    for (int vi = 0; vi < num_verts; ++vi) {
        cx += vertices[vi * 5 + 0];
        cy += vertices[vi * 5 + 1];
    }

    cx /= num_verts;
    cy /= num_verts;

    return glm::vec2(cx, cy);
}

void Tangram::handle_keys(const std::array<bool, 1024>& keys){
    if (keys[GLFW_KEY_TAB]) {
        if (!changed_selection) {
            selected_piece = (selected_piece + 1) % pieces.size();
            changed_selection = true;
        }
    } else {
        changed_selection = false;
    }

    if (keys[GLFW_KEY_UP]) piece_positions[selected_piece].y += move_step;
    if (keys[GLFW_KEY_DOWN]) piece_positions[selected_piece].y -= move_step;
    if (keys[GLFW_KEY_LEFT]) piece_positions[selected_piece].x -= move_step;
    if (keys[GLFW_KEY_RIGHT]) piece_positions[selected_piece].x += move_step;

    if (keys[GLFW_KEY_Q]) piece_rotations[selected_piece] += rotate_step;
    if (keys[GLFW_KEY_E]) piece_rotations[selected_piece] -= rotate_step;
}

void Tangram::render(const std::shared_ptr<Shader>& shader, glm::mat4& global_model)
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
        glm::mat4 model = global_model;
        model = glm::translate(model, glm::vec3(piece_positions[i], 0.0f));           // Move to position
        model = glm::translate(model, glm::vec3(piece_centers[i], 0.0f));             // Move center to origin
        model = glm::rotate(model, piece_rotations[i], glm::vec3(0, 0, 1));           // Rotate
        model = glm::translate(model, -glm::vec3(piece_centers[i], 0.0f));            // Move back
        glUniformMatrix4fv(shader->get_uniform_model_id(), 1, GL_FALSE, glm::value_ptr(model));

        // Highlight selected piece
        glm::vec3 color = colors[i];
        if (i != selected_piece) {
            color *= 0.8f; // Make it brighter
            color = glm::clamp(color, 0.0f, 1.0f); // Clamp to valid range
        }

        // Set color uniform
        GLint colorLoc = glGetUniformLocation(shader->get_program_id(), "piece_color");
        glUniform3fv(colorLoc, 1, glm::value_ptr(color));

        pieces[i]->render();
    }
}