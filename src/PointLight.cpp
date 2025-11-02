#include <PointLight.hpp>
#include <string>

#include <glm/gtc/type_ptr.hpp>

PointLight::PointLight(const glm::vec3& _position,
                       const glm::vec3& _ambient,
                       const glm::vec3& _diffuse,
                       const glm::vec3& _specular,
                       GLfloat _constant,
                       GLfloat _linear,
                       GLfloat _quadratic) noexcept
    : position{_position},
      ambient{_ambient},
      diffuse{_diffuse},
      specular{_specular},
      constant{_constant},
      linear{_linear},
      quadratic{_quadratic}
{
}

void PointLight::use(const std::shared_ptr<Shader>& shader, GLuint light_index) const noexcept
{
    std::string base_uniform = "pointLights[" + std::to_string(light_index) + "]";

    glUniform3fv(glGetUniformLocation(shader->get_program_id(), (base_uniform + ".position").c_str()), 1, glm::value_ptr(position));
    glUniform3fv(glGetUniformLocation(shader->get_program_id(), (base_uniform + ".ambient").c_str()), 1, glm::value_ptr(ambient));
    glUniform3fv(glGetUniformLocation(shader->get_program_id(), (base_uniform + ".diffuse").c_str()), 1, glm::value_ptr(diffuse));
    glUniform3fv(glGetUniformLocation(shader->get_program_id(), (base_uniform + ".specular").c_str()), 1, glm::value_ptr(specular));
    glUniform1f(glGetUniformLocation(shader->get_program_id(), (base_uniform + ".constant").c_str()), constant);
    glUniform1f(glGetUniformLocation(shader->get_program_id(), (base_uniform + ".linear").c_str()), linear);
    glUniform1f(glGetUniformLocation(shader->get_program_id(), (base_uniform + ".quadratic").c_str()), quadratic);
}