#ifndef TANGRAM_HPP
#define TANGRAM_HPP

#include <vector>
#include <memory>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <Texture.hpp>
#include <Mesh.hpp>
#include <GLFW/glfw3.h>
#include <array>

class Tangram
{
public:
    Tangram();
    void initialize();
    void update(float deltaTime, const std::array<bool, 1024>& keys); // ahora recibe el teclado
    void render(const std::shared_ptr<Shader> &shader);

    void select_piece(int index); // seleccionar pieza
    void deselect_piece();        // quitar selección
    bool has_selected_piece() const { return selected_piece != -1; }

private:
    std::vector<std::shared_ptr<Mesh>> pieces;
    std::vector<glm::mat4> transformations;
    int selected_piece = -1; // -1 = ninguna seleccionada

    void create_pieces();
};

#endif // TANGRAM_HPP