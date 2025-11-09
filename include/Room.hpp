#pragma once

#include <vector>
#include <memory>
#include <filesystem>

#include <glm/glm.hpp>

#include <Mesh.hpp>
#include <Texture.hpp>
#include <Shader.hpp>

class Room
{
public:
    Room(const std::filesystem::path& root_path);
    ~Room() = default;

    // void render(const std::shared_ptr<Shader>& shader);
    // Render the room with an explicit model matrix (allows placing multiple
    // copies of the room in the world).
    void render(const std::shared_ptr<Shader>& shader, const glm::mat4& model);

private:
    std::shared_ptr<Mesh> floor_mesh;
    std::shared_ptr<Mesh> wall_mesh;
    std::shared_ptr<Mesh> ceiling_mesh;
    std::shared_ptr<Texture> floor_texture;
    std::shared_ptr<Texture> floor_normal_texture;
    std::shared_ptr<Texture> wall_texture;
    std::shared_ptr<Texture> wall_normal_texture;
    std::shared_ptr<Texture> ceiling_texture;
};