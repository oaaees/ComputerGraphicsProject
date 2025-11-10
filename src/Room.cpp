#include <Room.hpp>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <functional>

Room::Room(const std::filesystem::path &root_path, int door_mask)
{
    // Load textures
    floor_texture = std::make_shared<Texture>(root_path / "textures" / "floor_albedo.jpg");
    floor_texture->load();

    floor_normal_texture = std::make_shared<Texture>(root_path / "textures" / "floor_normal.png");
    floor_normal_texture->load();

    wall_texture = std::make_shared<Texture>(root_path / "textures" / "wall_albedo.jpg");
    wall_texture->load();

    wall_normal_texture = std::make_shared<Texture>(root_path / "textures" / "wall_normal.png");
    wall_normal_texture->load();

    // --- Room Geometry ---

    // Floor
    std::vector<GLfloat> floor_vertices = {
        // Positions          // Normals           // Texture Coords
        -10.0f, -2.0f, -10.0f,  0.0f, 1.0f, 0.0f,   0.0f, 0.0f,
         10.0f, -2.0f, -10.0f,  0.0f, 1.0f, 0.0f,   5.0f, 0.0f,
         10.0f, -2.0f,  10.0f,  0.0f, 1.0f, 0.0f,   5.0f, 5.0f,
        -10.0f, -2.0f,  10.0f,  0.0f, 1.0f, 0.0f,   0.0f, 5.0f,
    };
    std::vector<unsigned int> floor_indices = {0, 2, 1, 0, 3, 2};
    floor_mesh = Mesh::create(floor_vertices, floor_indices);

    // Ceiling
    std::vector<GLfloat> ceiling_vertices = {
        // Positions          // Normals            // Texture Coords
        -10.0f,  8.0f, -10.0f,  0.0f, -1.0f, 0.0f,   0.0f, 0.0f,
         10.0f,  8.0f, -10.0f,  0.0f, -1.0f, 0.0f,   2.0f, 0.0f,
         10.0f,  8.0f,  10.0f,  0.0f, -1.0f, 0.0f,   2.0f, 1.0f,
        -10.0f,  8.0f,  10.0f,  0.0f, -1.0f, 0.0f,   0.0f, 1.0f,
    };
    std::vector<unsigned int> ceiling_indices = {0, 1, 2, 0, 2, 3}; // Reversed winding for downward normal
    ceiling_mesh = Mesh::create(ceiling_vertices, ceiling_indices);

        // Build each wall side as an independent mesh so walls can be rendered
        // separately (e.g. included in shadow/depth passes). We reuse the same
        // texture/normal map for all wall panels.

        // Common dimensions
        const float x_min = -10.0f, x_max = 10.0f;
        const float z_min = -10.0f, z_max = 10.0f;
        const float y_min = -2.0f, y_max = 8.0f;

        // Door geometry (centered on wall)
        const float door_width = 4.0f;
        const float door_height = 6.0f; // from y_min to y_min + door_height
        const float half_dw = door_width * 0.5f;
        const float door_top = y_min + door_height;

                auto makeSideMesh = [&](auto build_fn)
                {
                std::vector<GLfloat> side_vertices;
                std::vector<unsigned int> side_indices;
                unsigned int idx = 0;

                auto pushV = [&](const glm::vec3 &p, const glm::vec3 &n, const glm::vec2 &t)
                {
                        side_vertices.push_back(p.x);
                        side_vertices.push_back(p.y);
                        side_vertices.push_back(p.z);
                        side_vertices.push_back(n.x);
                        side_vertices.push_back(n.y);
                        side_vertices.push_back(n.z);
                        side_vertices.push_back(t.x);
                        side_vertices.push_back(t.y);
                };

                // Build a thin box from the given quad (a,b,c,d) with the provided
                // outward normal. This produces 8 vertices and 12 triangles (36
                // indices) per quad so walls have real thickness and cast
                // physically-correct shadows.
                const float wall_thickness = 0.08f; // meters; tweak if needed
                auto addQuadLocal = [&](const glm::vec3 &a, const glm::vec3 &b, const glm::vec3 &c, const glm::vec3 &d,
                                                                const glm::vec3 &normal, const glm::vec2 &ta, const glm::vec2 &tb, const glm::vec2 &tc, const glm::vec2 &td)
                {
                        glm::vec3 off = normal * (wall_thickness * 0.5f);
                        // front (towards normal)
                        glm::vec3 af = a + off;
                        glm::vec3 bf = b + off;
                        glm::vec3 cf = c + off;
                        glm::vec3 df = d + off;
                        // back (away from normal)
                        glm::vec3 ab = a - off;
                        glm::vec3 bb = b - off;
                        glm::vec3 cb = c - off;
                        glm::vec3 db = d - off;

                        // push front verts (use provided uvs)
                        pushV(af, normal, ta);
                        pushV(bf, normal, tb);
                        pushV(cf, normal, tc);
                        pushV(df, normal, td);

                        // push back verts (normals inverted)
                        pushV(ab, -normal, ta);
                        pushV(bb, -normal, tb);
                        pushV(cb, -normal, tc);
                        pushV(db, -normal, td);

                        // indices (base index)
                        // front face
                        side_indices.push_back(idx + 0);
                        side_indices.push_back(idx + 1);
                        side_indices.push_back(idx + 2);
                        side_indices.push_back(idx + 0);
                        side_indices.push_back(idx + 2);
                        side_indices.push_back(idx + 3);

                        // back face (reversed winding so normal points outward)
                        side_indices.push_back(idx + 4);
                        side_indices.push_back(idx + 6);
                        side_indices.push_back(idx + 5);
                        side_indices.push_back(idx + 4);
                        side_indices.push_back(idx + 7);
                        side_indices.push_back(idx + 6);

                        // side AB
                        side_indices.push_back(idx + 0);
                        side_indices.push_back(idx + 1);
                        side_indices.push_back(idx + 5);
                        side_indices.push_back(idx + 0);
                        side_indices.push_back(idx + 5);
                        side_indices.push_back(idx + 4);

                        // side BC
                        side_indices.push_back(idx + 1);
                        side_indices.push_back(idx + 2);
                        side_indices.push_back(idx + 6);
                        side_indices.push_back(idx + 1);
                        side_indices.push_back(idx + 6);
                        side_indices.push_back(idx + 5);

                        // side CD
                        side_indices.push_back(idx + 2);
                        side_indices.push_back(idx + 3);
                        side_indices.push_back(idx + 7);
                        side_indices.push_back(idx + 2);
                        side_indices.push_back(idx + 7);
                        side_indices.push_back(idx + 6);

                        // side DA
                        side_indices.push_back(idx + 3);
                        side_indices.push_back(idx + 0);
                        side_indices.push_back(idx + 4);
                        side_indices.push_back(idx + 3);
                        side_indices.push_back(idx + 4);
                        side_indices.push_back(idx + 7);

                        idx += 8;
                };

                // Let caller add quads to this side using addQuadLocal
                build_fn(addQuadLocal);

                if (!side_vertices.empty())
                {
                        // Create the front-facing mesh (original winding). This
                        // mesh will be used for shadow rendering.
                        auto m_front = Mesh::create(side_vertices, side_indices);
                        // visual: add the front face
                        walls.push_back(std::make_shared<Wall>(m_front, wall_texture, wall_normal_texture));
                        // shadow: front face should cast shadows
                        shadow_walls.push_back(std::make_shared<Wall>(m_front, wall_texture, wall_normal_texture));

                        // Build back-face geometry (negated normals, reversed winding)
                        size_t vert_count = side_vertices.size() / 8; // 8 floats per input vertex
                        std::vector<GLfloat> back_vertices;
                        back_vertices.reserve(side_vertices.size());
                        for (size_t vi = 0; vi < vert_count; ++vi)
                        {
                                // pos
                                back_vertices.push_back(side_vertices[vi * 8 + 0]);
                                back_vertices.push_back(side_vertices[vi * 8 + 1]);
                                back_vertices.push_back(side_vertices[vi * 8 + 2]);
                                // normal (negated)
                                back_vertices.push_back(-side_vertices[vi * 8 + 3]);
                                back_vertices.push_back(-side_vertices[vi * 8 + 4]);
                                back_vertices.push_back(-side_vertices[vi * 8 + 5]);
                                // uv
                                back_vertices.push_back(side_vertices[vi * 8 + 6]);
                                back_vertices.push_back(side_vertices[vi * 8 + 7]);
                        }

                        std::vector<unsigned int> back_indices;
                        back_indices.reserve(side_indices.size());
                        for (size_t ii = 0; ii < side_indices.size(); ii += 3)
                        {
                                unsigned int a = side_indices[ii + 0];
                                unsigned int b = side_indices[ii + 1];
                                unsigned int c = side_indices[ii + 2];
                                // reversed winding for back face
                                back_indices.push_back(c);
                                back_indices.push_back(b);
                                back_indices.push_back(a);
                        }

                        auto m_back = Mesh::create(back_vertices, back_indices);
                        // visual: add the back face mesh, but do NOT add it to
                        // shadow_walls so it won't write into the depth cubemap.
                        walls.push_back(std::make_shared<Wall>(m_back, wall_texture, wall_normal_texture));
                }
        };

        // BACK wall (z = z_min). Normal towards +Z
        if ((door_mask & Room::DOOR_BACK) == 0)
        {
                makeSideMesh([&](auto addQuadLocal){
                        addQuadLocal(glm::vec3(x_min, y_min, z_min), glm::vec3(x_max, y_min, z_min), glm::vec3(x_max, y_max, z_min), glm::vec3(x_min, y_max, z_min),
                                                 glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 0.0f), glm::vec2(2.0f, 0.0f), glm::vec2(2.0f, 1.0f), glm::vec2(0.0f, 1.0f));
                });
        }
        else
        {
                makeSideMesh([&](auto addQuadLocal){
                        addQuadLocal(glm::vec3(x_min, y_min, z_min), glm::vec3(-half_dw, y_min, z_min), glm::vec3(-half_dw, y_max, z_min), glm::vec3(x_min, y_max, z_min),
                                                 glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 0.0f), glm::vec2(1.0f, 1.0f), glm::vec2(0.0f, 1.0f));
                        addQuadLocal(glm::vec3(half_dw, y_min, z_min), glm::vec3(x_max, y_min, z_min), glm::vec3(x_max, y_max, z_min), glm::vec3(half_dw, y_max, z_min),
                                                 glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 0.0f), glm::vec2(1.0f, 1.0f), glm::vec2(0.0f, 1.0f));
                        addQuadLocal(glm::vec3(-half_dw, door_top, z_min), glm::vec3(half_dw, door_top, z_min), glm::vec3(half_dw, y_max, z_min), glm::vec3(-half_dw, y_max, z_min),
                                                 glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 0.0f), glm::vec2(1.0f, 1.0f), glm::vec2(0.0f, 1.0f));
                });
        }

        // FRONT wall (z = z_max). Normal towards -Z
        if ((door_mask & Room::DOOR_FRONT) == 0)
        {
                makeSideMesh([&](auto addQuadLocal){
                        addQuadLocal(glm::vec3(x_min, y_min, z_max), glm::vec3(x_max, y_min, z_max), glm::vec3(x_max, y_max, z_max), glm::vec3(x_min, y_max, z_max),
                                                 glm::vec3(0.0f, 0.0f, -1.0f), glm::vec2(2.0f, 0.0f), glm::vec2(0.0f, 0.0f), glm::vec2(0.0f, 1.0f), glm::vec2(2.0f, 1.0f));
                });
        }
        else
        {
                makeSideMesh([&](auto addQuadLocal){
                        addQuadLocal(glm::vec3(x_min, y_min, z_max), glm::vec3(-half_dw, y_min, z_max), glm::vec3(-half_dw, y_max, z_max), glm::vec3(x_min, y_max, z_max),
                                                 glm::vec3(0.0f, 0.0f, -1.0f), glm::vec2(2.0f, 0.0f), glm::vec2(1.0f, 0.0f), glm::vec2(1.0f, 1.0f), glm::vec2(2.0f, 1.0f));
                        addQuadLocal(glm::vec3(half_dw, y_min, z_max), glm::vec3(x_max, y_min, z_max), glm::vec3(x_max, y_max, z_max), glm::vec3(half_dw, y_max, z_max),
                                                 glm::vec3(0.0f, 0.0f, -1.0f), glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 0.0f), glm::vec2(1.0f, 1.0f), glm::vec2(0.0f, 1.0f));
                        addQuadLocal(glm::vec3(-half_dw, door_top, z_max), glm::vec3(half_dw, door_top, z_max), glm::vec3(half_dw, y_max, z_max), glm::vec3(-half_dw, y_max, z_max),
                                                 glm::vec3(0.0f, 0.0f, -1.0f), glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 0.0f), glm::vec2(1.0f, 1.0f), glm::vec2(0.0f, 1.0f));
                });
        }

        // LEFT wall (x = x_min). Normal towards +X
        if ((door_mask & Room::DOOR_LEFT) == 0)
        {
                makeSideMesh([&](auto addQuadLocal){
                        addQuadLocal(glm::vec3(x_min, y_min, z_max), glm::vec3(x_min, y_min, z_min), glm::vec3(x_min, y_max, z_min), glm::vec3(x_min, y_max, z_max),
                                                 glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(2.0f, 0.0f), glm::vec2(0.0f, 0.0f), glm::vec2(0.0f, 1.0f), glm::vec2(2.0f, 1.0f));
                });
        }
        else
        {
                makeSideMesh([&](auto addQuadLocal){
                        addQuadLocal(glm::vec3(x_min, y_min, z_min), glm::vec3(x_min, y_min, -half_dw), glm::vec3(x_min, y_max, -half_dw), glm::vec3(x_min, y_max, z_min),
                                                 glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 0.0f), glm::vec2(1.0f, 1.0f), glm::vec2(0.0f, 1.0f));
                        addQuadLocal(glm::vec3(x_min, y_min, half_dw), glm::vec3(x_min, y_min, z_max), glm::vec3(x_min, y_max, z_max), glm::vec3(x_min, y_max, half_dw),
                                                 glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 0.0f), glm::vec2(1.0f, 1.0f), glm::vec2(0.0f, 1.0f));
                        addQuadLocal(glm::vec3(x_min, door_top, -half_dw), glm::vec3(x_min, door_top, half_dw), glm::vec3(x_min, y_max, half_dw), glm::vec3(x_min, y_max, -half_dw),
                                                 glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 0.0f), glm::vec2(1.0f, 1.0f), glm::vec2(0.0f, 1.0f));
                });
        }

        // RIGHT wall (x = x_max). Normal towards -X
        if ((door_mask & Room::DOOR_RIGHT) == 0)
        {
                makeSideMesh([&](auto addQuadLocal){
                        addQuadLocal(glm::vec3(x_max, y_min, z_min), glm::vec3(x_max, y_min, z_max), glm::vec3(x_max, y_max, z_max), glm::vec3(x_max, y_max, z_min),
                                                 glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec2(2.0f, 0.0f), glm::vec2(0.0f, 0.0f), glm::vec2(0.0f, 1.0f), glm::vec2(2.0f, 1.0f));
                });
        }
        else
        {
                makeSideMesh([&](auto addQuadLocal){
                        addQuadLocal(glm::vec3(x_max, y_min, z_min), glm::vec3(x_max, y_min, -half_dw), glm::vec3(x_max, y_max, -half_dw), glm::vec3(x_max, y_max, z_min),
                                                 glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 0.0f), glm::vec2(1.0f, 1.0f), glm::vec2(0.0f, 1.0f));
                        addQuadLocal(glm::vec3(x_max, y_min, half_dw), glm::vec3(x_max, y_min, z_max), glm::vec3(x_max, y_max, z_max), glm::vec3(x_max, y_max, half_dw),
                                                 glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 0.0f), glm::vec2(1.0f, 1.0f), glm::vec2(0.0f, 1.0f));
                        addQuadLocal(glm::vec3(x_max, door_top, -half_dw), glm::vec3(x_max, door_top, half_dw), glm::vec3(x_max, y_max, half_dw), glm::vec3(x_max, y_max, -half_dw),
                                                 glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 0.0f), glm::vec2(1.0f, 1.0f), glm::vec2(0.0f, 1.0f));
                });
        }
}

void Room::render(const std::shared_ptr<Shader> &shader, const glm::mat4 &model)
{
    glUniformMatrix4fv(shader->get_uniform_model_id(), 1, GL_FALSE, glm::value_ptr(model));

    // Render floor (more shiny)
    glUniform1f(glGetUniformLocation(shader->get_program_id(), "material.shininess"), 32.0f);
    floor_texture->use(); // Binds to GL_TEXTURE0
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, floor_normal_texture->get_id());
    floor_mesh->render();

    // Render ceiling (less shiny)
    glUniform1f(glGetUniformLocation(shader->get_program_id(), "material.shininess"), 16.0f);
    wall_texture->use(); // Use the wall's albedo texture
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, wall_normal_texture->get_id()); // Use the wall's normal map
    ceiling_mesh->render();

        // Render walls (matte). Each wall is a Wall object and may contain
        // one or more panels (for door openings). Rendering them individually
        // ensures they are included in shadow/depth passes when needed.
        for (const auto &w : walls)
        {
                if (w)
                        w->render(shader, model);
        }
}

void Room::render_for_depth(const std::shared_ptr<Shader> &shader, const glm::mat4 &model)
{
    glUniformMatrix4fv(shader->get_uniform_model_id(), 1, GL_FALSE, glm::value_ptr(model));

    // Render floor and ceiling into the depth map
    floor_mesh->render();
    ceiling_mesh->render();

    // Render only shadow-casting walls (front faces)
    for (const auto &w : shadow_walls)
    {
        if (w)
            w->render(shader, model);
    }
}