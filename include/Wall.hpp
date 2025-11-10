#pragma once

#include <memory>
#include <glm/glm.hpp>
#include <Mesh.hpp>
#include <Texture.hpp>
#include <Shader.hpp>

// Simple wrapper around a mesh + textures representing a single wall (or wall panel)
class Wall
{
public:
    Wall(std::shared_ptr<Mesh> mesh, std::shared_ptr<Texture> albedo, std::shared_ptr<Texture> normal);
    ~Wall() = default;

    // Render the wall with a shader and model matrix
    void render(const std::shared_ptr<Shader> &shader, const glm::mat4 &model) const;

    std::shared_ptr<Mesh> get_mesh() const { return mesh; }

private:
    std::shared_ptr<Mesh> mesh;
    std::shared_ptr<Texture> albedo;
    std::shared_ptr<Texture> normal;
};
