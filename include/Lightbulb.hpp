#pragma once

#include <memory>
#include <vector>
#include <GL/glew.h>

#include <glm/glm.hpp>

#include <PointLight.hpp>
#include <Mesh.hpp>
#include <Shader.hpp>

class Lightbulb
{
public:
    Lightbulb(const PointLight& light, const glm::vec3& color);

    void render(const std::shared_ptr<Shader>& shader) const;
    void use_light(const std::shared_ptr<Shader>& shader, GLuint light_index) const;
    void set_position(const glm::vec3& p);

    static void create_mesh();

private:
    PointLight point_light;
    glm::vec3 bulb_color;

    static std::shared_ptr<Mesh> mesh;
};