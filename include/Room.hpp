#pragma once

#include <vector>
#include <memory>
#include <filesystem>

#include <glm/glm.hpp>

#include <Mesh.hpp>
#include <Texture.hpp>
#include <Shader.hpp>
#include <Wall.hpp>

class Room
{
public:
    enum DoorSide
    {
        DOOR_NONE = 0,
        DOOR_FRONT = 1 << 0, // +Z
        DOOR_BACK = 1 << 1,  // -Z
        DOOR_LEFT = 1 << 2,  // -X
        DOOR_RIGHT = 1 << 3  // +X
    };

    // root_path: directory where textures/ live. door_mask: bitmask of DoorSide
    Room(const std::filesystem::path &root_path, int door_mask = DOOR_NONE);
    ~Room() = default;

    // Render the room with an explicit model matrix (allows placing multiple
    // copies of the room in the world).
    void render(const std::shared_ptr<Shader> &shader, const glm::mat4 &model);

private:
    std::shared_ptr<Mesh> floor_mesh;
    // Each side (or panel) is represented by a Wall so we can render them
    // individually (useful for shadow passes).
    std::vector<std::shared_ptr<Wall>> walls;
    std::shared_ptr<Mesh> ceiling_mesh;
    std::shared_ptr<Texture> floor_texture;
    std::shared_ptr<Texture> floor_normal_texture;
    std::shared_ptr<Texture> wall_texture;
    std::shared_ptr<Texture> wall_normal_texture;
    std::shared_ptr<Texture> ceiling_texture;
};