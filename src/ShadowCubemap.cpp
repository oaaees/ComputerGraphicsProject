#include <ShadowCubemap.hpp>

#include <glm/gtc/matrix_transform.hpp>

#include <iostream>

ShadowCubemap::ShadowCubemap(unsigned int size, float far_plane_) noexcept
    : map_size(size), far_plane(far_plane_)
{
    glGenFramebuffers(1, &depth_map_fbo);

    glGenTextures(1, &depth_cubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, depth_cubemap);
    for (unsigned int i = 0; i < 6; ++i) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT, map_size, map_size, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    // Configure framebuffer: no color buffer is drawn to.
    glBindFramebuffer(GL_FRAMEBUFFER, depth_map_fbo);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Unbind texture
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

ShadowCubemap::~ShadowCubemap()
{
    if (depth_cubemap) glDeleteTextures(1, &depth_cubemap);
    if (depth_map_fbo) glDeleteFramebuffers(1, &depth_map_fbo);
}

std::vector<glm::mat4> ShadowCubemap::get_shadow_matrices(const glm::vec3& light_pos, float near_plane) const noexcept
{
    std::vector<glm::mat4> matrices;
    glm::mat4 shadow_proj = glm::perspective(glm::radians(90.0f), 1.0f, near_plane, far_plane);

    matrices.push_back(shadow_proj * glm::lookAt(light_pos, light_pos + glm::vec3( 1.0,  0.0,  0.0), glm::vec3(0.0, -1.0,  0.0)));
    matrices.push_back(shadow_proj * glm::lookAt(light_pos, light_pos + glm::vec3(-1.0,  0.0,  0.0), glm::vec3(0.0, -1.0,  0.0)));
    matrices.push_back(shadow_proj * glm::lookAt(light_pos, light_pos + glm::vec3( 0.0,  1.0,  0.0), glm::vec3(0.0,  0.0,  1.0)));
    matrices.push_back(shadow_proj * glm::lookAt(light_pos, light_pos + glm::vec3( 0.0, -1.0,  0.0), glm::vec3(0.0,  0.0, -1.0)));
    matrices.push_back(shadow_proj * glm::lookAt(light_pos, light_pos + glm::vec3( 0.0,  0.0,  1.0), glm::vec3(0.0, -1.0,  0.0)));
    matrices.push_back(shadow_proj * glm::lookAt(light_pos, light_pos + glm::vec3( 0.0,  0.0, -1.0), glm::vec3(0.0, -1.0,  0.0)));

    return matrices;
}

std::vector<glm::mat4> ShadowCubemap::get_shadow_views(const glm::vec3& light_pos) const noexcept
{
    std::vector<glm::mat4> views;
    views.push_back(glm::lookAt(light_pos, light_pos + glm::vec3( 1.0,  0.0,  0.0), glm::vec3(0.0, -1.0,  0.0)));
    views.push_back(glm::lookAt(light_pos, light_pos + glm::vec3(-1.0,  0.0,  0.0), glm::vec3(0.0, -1.0,  0.0)));
    views.push_back(glm::lookAt(light_pos, light_pos + glm::vec3( 0.0,  1.0,  0.0), glm::vec3(0.0,  0.0,  1.0)));
    views.push_back(glm::lookAt(light_pos, light_pos + glm::vec3( 0.0, -1.0,  0.0), glm::vec3(0.0,  0.0, -1.0)));
    views.push_back(glm::lookAt(light_pos, light_pos + glm::vec3( 0.0,  0.0,  1.0), glm::vec3(0.0, -1.0,  0.0)));
    views.push_back(glm::lookAt(light_pos, light_pos + glm::vec3( 0.0,  0.0, -1.0), glm::vec3(0.0, -1.0,  0.0)));
    return views;
}
