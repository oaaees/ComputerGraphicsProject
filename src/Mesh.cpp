#include <Mesh.hpp>
#include <glm/glm.hpp>

std::shared_ptr<Mesh> Mesh::create(const std::vector<GLfloat>& vertices, std::vector<unsigned int> indices) noexcept
{
    auto mesh = std::make_shared<Mesh>();

    mesh->index_count = indices.size();

    std::vector<GLfloat> final_vertices;
    std::vector<glm::vec3> tangents(vertices.size() / 8, glm::vec3(0.0f));

    // Calculate tangents
    for (size_t i = 0; i < indices.size(); i += 3) {
        unsigned int i0 = indices[i];
        unsigned int i1 = indices[i+1];
        unsigned int i2 = indices[i+2];

        glm::vec3 v0(vertices[i0 * 8], vertices[i0 * 8 + 1], vertices[i0 * 8 + 2]);
        glm::vec3 v1(vertices[i1 * 8], vertices[i1 * 8 + 1], vertices[i1 * 8 + 2]);
        glm::vec3 v2(vertices[i2 * 8], vertices[i2 * 8 + 1], vertices[i2 * 8 + 2]);

        glm::vec2 uv0(vertices[i0 * 8 + 6], vertices[i0 * 8 + 7]);
        glm::vec2 uv1(vertices[i1 * 8 + 6], vertices[i1 * 8 + 7]);
        glm::vec2 uv2(vertices[i2 * 8 + 6], vertices[i2 * 8 + 7]);

        glm::vec3 edge1 = v1 - v0;
        glm::vec3 edge2 = v2 - v0;
        glm::vec2 deltaUV1 = uv1 - uv0;
        glm::vec2 deltaUV2 = uv2 - uv0;

        float denom = (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
        // Guard against degenerate UVs (zero denominator). If degenerate,
        // skip this triangle's tangent contribution. We'll synthesize a
        // fallback tangent per-vertex later when needed.
        if (fabs(denom) > 1e-6f)
        {
            float f = 1.0f / denom;

            glm::vec3 tangent;
            tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
            tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
            tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

            tangents[i0] += tangent;
            tangents[i1] += tangent;
            tangents[i2] += tangent;
        }
    }

    // Build final vertex buffer with tangents
    final_vertices.reserve(vertices.size() / 8 * 11);
    for(size_t i = 0; i < vertices.size() / 8; ++i) {
        glm::vec3 n(vertices[i * 8 + 3], vertices[i * 8 + 4], vertices[i * 8 + 5]);
        glm::vec3 t = tangents[i];
        // If accumulated tangent is (near) zero (e.g. degenerate UVs),
        // synthesize a stable tangent perpendicular to the normal. This
        // prevents NaNs when normal mapping procedurally-generated
        // geometry with poor UVs (thin boxes / reused UVs).
        if (glm::length(t) < 1e-6f)
        {
            // Pick an arbitrary vector that's not parallel to n
            glm::vec3 arbitrary = (fabs(n.x) < 0.9f) ? glm::vec3(1.0f, 0.0f, 0.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
            t = glm::normalize(glm::cross(arbitrary, n));
        }
        else
        {
            t = glm::normalize(t);
        }
        // Gram-Schmidt orthogonalize
        t = glm::normalize(t - glm::dot(t, n) * n);

        final_vertices.insert(final_vertices.end(), vertices.begin() + i * 8, vertices.begin() + i * 8 + 8);
        final_vertices.push_back(t.x);
        final_vertices.push_back(t.y);
        final_vertices.push_back(t.z);
    }

    glGenVertexArrays(1, &mesh->VAO_id);
    glBindVertexArray(mesh->VAO_id);

    glGenBuffers(1, &mesh->IBO_id);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->IBO_id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &mesh->VBO_id);
    glBindBuffer(GL_ARRAY_BUFFER, mesh->VBO_id);
    glBufferData(GL_ARRAY_BUFFER, final_vertices.size() * sizeof(GLfloat), final_vertices.data(), GL_STATIC_DRAW);

    GLsizei stride = sizeof(GLfloat) * 11; // 3 pos, 3 normal, 2 uv, 3 tangent
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, nullptr);
    glEnableVertexAttribArray(0);
    // Normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(sizeof(GLfloat) * 3));
    glEnableVertexAttribArray(1);
    // Texture Coordinate attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(sizeof(GLfloat) * 6));
    glEnableVertexAttribArray(2);
    // Tangent attribute
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(sizeof(GLfloat) * 8));
    glEnableVertexAttribArray(3);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    glBindVertexArray(0);

    return mesh;
}

Mesh::~Mesh()
{
    clear();
}

void Mesh::render() const noexcept
{
    glBindVertexArray(VAO_id);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO_id);
    glDrawElements(GL_TRIANGLES, index_count, GL_UNSIGNED_INT, NULL);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Mesh::clear() noexcept
{
    if (IBO_id != 0)
    {
        glDeleteBuffers(1, &IBO_id);
        IBO_id = 0;
    }

    if (VBO_id != 0)
    {
        glDeleteBuffers(1, &VBO_id);
        VBO_id = 0;
    }

    if (VAO_id != 0)
    {
        glDeleteVertexArrays(1, &VAO_id);
        VAO_id = 0;
    }
}
