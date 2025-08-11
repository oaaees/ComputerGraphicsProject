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

class Tangram{
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

#endif // TANGRAM_HPP