#pragma once

#include <glm/glm.hpp>
#include <GL/glew.h>
#include <vector>

class ShadowCubemap
{
public:
    ShadowCubemap(unsigned int size, float far_plane) noexcept;
    ~ShadowCubemap();

    GLuint get_depth_cubemap_id() const noexcept { return depth_cubemap; }
    GLuint get_fbo() const noexcept { return depth_map_fbo; }
    float get_far_plane() const noexcept { return far_plane; }
    unsigned int get_size() const noexcept { return map_size; }

    // Returns projection*view matrices for the 6 cubemap faces for a point light at light_pos
    std::vector<glm::mat4> get_shadow_matrices(const glm::vec3& light_pos, float near_plane) const noexcept;
    // Returns only the view matrices (lookAt) for the 6 cubemap faces
    std::vector<glm::mat4> get_shadow_views(const glm::vec3& light_pos) const noexcept;

private:
    GLuint depth_map_fbo{0};
    GLuint depth_cubemap{0};
    unsigned int map_size{1024};
    float far_plane{25.0f};
};
