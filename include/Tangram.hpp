#ifndef TANGRAM_HPP
#define TANGRAM_HPP

#include <vector>
#include <array>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <Texture.hpp>
#include <Mesh.hpp>
#include <Shader.hpp>

class Tangram{
    public:
        Tangram();
        void render(const std::shared_ptr<Shader>& shader, glm::mat4& global_model);
        void handle_keys(const std::array<bool, 1024>& keys);
        glm::vec2 calc_piece_center(std::vector<GLfloat> vertices);

    private:
        std::vector<std::shared_ptr<Mesh>> pieces;
        std::vector<glm::vec2> piece_centers;
        std::vector<glm::mat4> transformations;
        void create_pieces();

        int selected_piece = 0;
        const float move_step = 0.01f;
        const float rotate_step = glm::radians(1.0f);

        std::vector<glm::vec2> piece_positions; // x, y position for each piece
        std::vector<float> piece_rotations;     // rotation angle for each piece

        bool changed_selection = false;
};

#endif // TANGRAM_HPP