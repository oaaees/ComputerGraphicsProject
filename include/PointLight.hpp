#pragma once

#include <memory>

#include <GL/glew.h>
#include <glm/glm.hpp>

#include <Shader.hpp>

class PointLight
{
public:
    PointLight(const glm::vec3& _position,
               const glm::vec3& _ambient,
               const glm::vec3& _diffuse,
               const glm::vec3& _specular,
               GLfloat _constant,
               GLfloat _linear,
               GLfloat _quadratic) noexcept;

    void use(const std::shared_ptr<Shader>& shader, GLuint light_index) const noexcept;

    glm::vec3 get_position() const noexcept { return position; }

private:
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    // Attenuation
    GLfloat constant;
    GLfloat linear;
    GLfloat quadratic;
};