#pragma once

#include <memory>
#include <vector>
#include <filesystem>

#include <Mesh.hpp>
#include <Texture.hpp>
#include <glm/glm.hpp>

namespace AssimpLoader
{
    // A small renderable bundle: geometry + optional textures + local transform
    struct Renderable {
        std::shared_ptr<Mesh> mesh;
        std::shared_ptr<Texture> albedo; // may be nullptr
        std::shared_ptr<Texture> normal; // may be nullptr
        glm::mat4 transform{1.0f};
    };

    // Load a model file (e.g., .gltf) and return one or more Renderable objects
    // ready to be rendered by your existing system. Each Renderable.mesh will
    // contain vertices in the format expected by Mesh::create: pos(3), normal(3), uv(2).
    std::vector<Renderable> loadModel(const std::filesystem::path& path) noexcept;
}
