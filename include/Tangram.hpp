#ifndef TANGRAM_HPP
#define TANGRAM_HPP

#include <vector>
#include <memory>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <Texture.hpp>
#include <Mesh.hpp>

class Tangram
{
public:
    Tangram();

    void initialize();
    void update(float deltaTime);
    void render(const std::shared_ptr<Shader>& shader);

    void handle_input(int key, int action);
    void select_piece(double mouse_x, double mouse_y);
    void release_piece();
    
private:
    std::vector<std::shared_ptr<Mesh>> pieces;
    std::vector<Texture> textures;
    glm::mat4 model_matrix;

    std::vector<glm::vec2> piece_positions; // Add this for 2D positions
    int selected_piece = -1;
    bool dragging = false;

    void create_pieces();
    void load_textures();
};

#endif // TANGRAM_HPP