#include <Frustum.hpp>

void Frustum::update(const glm::mat4 &viewProjMatrix)
{
  glm::mat4 m = glm::transpose(viewProjMatrix);

  // Left plane
  planes[0] = m[3] + m[0];
  // Right plane
  planes[1] = m[3] - m[0];
  // Bottom plane
  planes[2] = m[3] + m[1];
  // Top plane
  planes[3] = m[3] - m[1];
  // Near plane
  planes[4] = m[3] + m[2];
  // Far plane
  planes[5] = m[3] - m[2];

  // Normalize planes
  for (int i = 0; i < 6; i++)
  {
    float length = glm::length(glm::vec3(planes[i]));
    planes[i] /= length;
  }
}

bool Frustum::isSphereInFrustum(const glm::vec3 &center, float radius) const
{
  for (int i = 0; i < 6; i++)
  {
    float distance = glm::dot(glm::vec3(planes[i]), center) + planes[i].w;
    if (distance < -radius)
    {
      return false;
    }
  }
  return true;
}

bool Frustum::isAABBInFrustum(const glm::vec3 &min, const glm::vec3 &max) const
{
  for (int i = 0; i < 6; i++)
  {
    glm::vec3 positive = min;
    if (planes[i].x >= 0)
      positive.x = max.x;
    if (planes[i].y >= 0)
      positive.y = max.y;
    if (planes[i].z >= 0)
      positive.z = max.z;

    float distance = glm::dot(glm::vec3(planes[i]), positive) + planes[i].w;
    if (distance < 0)
    {
      return false;
    }
  }
  return true;
}