#pragma once

#include <glm/glm.hpp>

class Frustum {
public:
    void update(const glm::mat4& viewProjMatrix);
    bool isSphereInFrustum(const glm::vec3& center, float radius) const;
    bool isAABBInFrustum(const glm::vec3& min, const glm::vec3& max) const;
    
private:
    glm::vec4 planes[6];
};