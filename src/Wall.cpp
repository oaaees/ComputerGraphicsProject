#include <Wall.hpp>
#include <glm/gtc/type_ptr.hpp>

Wall::Wall(std::shared_ptr<Mesh> mesh_, std::shared_ptr<Texture> albedo_, std::shared_ptr<Texture> normal_)
    : mesh(std::move(mesh_)), albedo(std::move(albedo_)), normal(std::move(normal_))
{
}

void Wall::render(const std::shared_ptr<Shader> &shader, const glm::mat4 &model) const
{
    glUniformMatrix4fv(shader->get_uniform_model_id(), 1, GL_FALSE, glm::value_ptr(model));

    // Use wall material (matte-ish)
    glUniform1f(glGetUniformLocation(shader->get_program_id(), "material.shininess"), 64.0f);

    if (albedo)
        albedo->use();
    if (normal)
    {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, normal->get_id());
    }

    if (mesh)
        mesh->render();
}
