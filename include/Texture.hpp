#pragma once

#include <filesystem>

#include <GL/glew.h>

#include <stb_image.h>

#include <BSlogger.hpp>

class Texture
{
public:
    Texture() = default;

    Texture(const std::filesystem::path& _file_path);
    // Create a 1x1 solid color texture (r,g,b,a) in [0..255]
    Texture(unsigned char r, unsigned char g, unsigned char b, unsigned char a = 255);

    ~Texture();

    void load() noexcept;

    void use() const noexcept;

    GLuint get_id() const noexcept { return id; }

private:
    void clear() noexcept;

    GLuint id{0};
    int width{0};
    int height{0};
    int bit_depth{0};
    std::filesystem::path file_path;
    // If true, create a 1x1 solid color texture instead of loading from file.
    bool solid_color{false};
    unsigned char solid_rgba[4]{255,255,255,255};
};