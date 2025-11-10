#include <iostream>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <Camera.hpp>
#include <Mesh.hpp>
#include <Shader.hpp>
#include <Window.hpp>
#include <Room.hpp>
#include <PointLight.hpp>
#include <Lightbulb.hpp>
#include <AssimpLoader.hpp>
#include <Texture.hpp>
#include <ShadowCubemap.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace fs = std::filesystem;

struct Data
{
    static std::vector<std::shared_ptr<Shader>> shader_list;
    static const fs::path root_path;
    static const fs::path vertex_shader_path;
    static const fs::path lightbulb_vertex_shader_path;
    static const fs::path fragment_shader_path;
    static const fs::path lightbulb_fragment_shader_path;
};

std::vector<std::shared_ptr<Shader>> Data::shader_list{};
const fs::path Data::root_path{fs::path{__FILE__}.parent_path()};
const fs::path Data::vertex_shader_path{Data::root_path / "shaders" / "shader.vert"};
const fs::path Data::lightbulb_vertex_shader_path{Data::root_path / "shaders" / "lightbulb.vert"};
const fs::path Data::fragment_shader_path{Data::root_path / "shaders" / "shader.frag"};
const fs::path Data::lightbulb_fragment_shader_path{Data::root_path / "shaders" / "lightbulb.frag"};

void create_shaders_program() noexcept
{
    Data::shader_list.push_back(Shader::create_from_files(Data::vertex_shader_path, Data::fragment_shader_path));
    Data::shader_list.push_back(Shader::create_from_files(Data::lightbulb_vertex_shader_path, Data::lightbulb_fragment_shader_path));
}

int main()
{
    // Window dimensions
    constexpr GLint WIDTH = 1200;
    constexpr GLint HEIGHT = 800;

    auto main_window = Window::create(WIDTH, HEIGHT, "The Room");

    if (main_window == nullptr)
    {
        return EXIT_FAILURE;
    }

    create_shaders_program();

    // Place camera inside the room
    Camera camera{glm::vec3{0.f, 1.f, 0.f}, glm::vec3{0.f, 1.f, 0.f}, -90.f, 0.f, 5.f, 0.15f};

    // Create the rooms (center + four directions). Each room can have openings
    // (doors) on specific walls so adjacent rooms connect.
    std::vector<Room> rooms;
    // center: open all four sides so it connects to each neighbor
    rooms.emplace_back(Data::root_path, Room::DOOR_FRONT | Room::DOOR_BACK | Room::DOOR_LEFT | Room::DOOR_RIGHT);
    // +X room: open left side (connects to center's right)
    rooms.emplace_back(Data::root_path, Room::DOOR_LEFT);
    // -X room: open right side (connects to center's left)
    rooms.emplace_back(Data::root_path, Room::DOOR_RIGHT);
    // +Z room: open back side (connects to center's front)
    rooms.emplace_back(Data::root_path, Room::DOOR_BACK);
    // -Z room: open front side (connects to center's back)
    rooms.emplace_back(Data::root_path, Room::DOOR_FRONT);

    // Prepare transforms for multiple room instances (museum layout)
    // We'll place 5 rooms in a cross layout: center + four directions.
    std::vector<glm::mat4> roomTransforms;
    const float roomSpacing = 20.0f;                                                                // spacing between room centers; tune as needed
    roomTransforms.push_back(glm::mat4(1.0f));                                                      // center
    roomTransforms.push_back(glm::translate(glm::mat4(1.0f), glm::vec3(roomSpacing, 0.0f, 0.0f)));  // +X
    roomTransforms.push_back(glm::translate(glm::mat4(1.0f), glm::vec3(-roomSpacing, 0.0f, 0.0f))); // -X
    roomTransforms.push_back(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, roomSpacing)));  // +Z
    roomTransforms.push_back(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -roomSpacing))); // -Z

    // Create the shared mesh for all lightbulbs
    Lightbulb::create_mesh();

    // Create fallback textures (white albedo and neutral normal map)
    auto fallback_albedo = std::make_shared<Texture>(255, 255, 255, 255);
    fallback_albedo->load();
    auto fallback_normal = std::make_shared<Texture>(128, 128, 255, 255);
    fallback_normal->load();

    // Load a simple test model (wooden_table_02_1k.gltf) using the Assimp loader
    std::vector<AssimpLoader::Renderable> imported_models = AssimpLoader::loadModel(Data::root_path / "models" / "wooden_table_02_1k.gltf");

    // Load props (one model per table). Expect these files to exist in models/
    std::vector<std::vector<AssimpLoader::Renderable>> prop_models;
    std::vector<std::string> prop_files = {
        "lubricant_spray_1k.gltf",
        "marble_bust_01_1k.gltf",
        "rubber_duck_toy_1k.gltf",
        "street_rat_1k.gltf"};
    for (auto &pf : prop_files)
    {
        auto group = AssimpLoader::loadModel(Data::root_path / "models" / pf);
        prop_models.push_back(std::move(group));
    }

    // Create a single lightbulb in the middle of the ceiling (we'll shadow this one)
    std::vector<Lightbulb> lightbulbs;
    PointLight ceilingLight(glm::vec3{0.0f, 7.5f, 0.0f},
                            glm::vec3{0.05f, 0.05f, 0.05f}, glm::vec3{2.0f, 2.0f, 2.0f}, glm::vec3{0.5f, 0.5f, 0.5f},
                            1.0f, 0.09f, 0.032f);
    lightbulbs.emplace_back(ceilingLight, glm::vec3{1.0f, 1.0f, 1.0f});

    // Shadow cubemap for the ceiling bulb (resolution, far plane tuned to scene)
    unsigned int SHADOW_SIZE = 2048;
    const float SHADOW_FAR = 25.0f;
    // Ensure no textures are bound to any unit (prevents sampler validation warnings)
    for (int i = 0; i < 8; ++i)
    {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    }

    auto depthShader = Shader::create_from_files(Data::root_path / "shaders" / "depth_cube.vert", Data::root_path / "shaders" / "depth_cube.frag");
    ShadowCubemap shadowCubemap(SHADOW_SIZE, SHADOW_FAR);

    // Runtime shadow controls were removed (use fixed parameters)
    const bool enableShadows = true;

    glm::mat4 projection = glm::perspective(45.f, main_window->get_aspect_ratio(), 0.1f, 100.f);

    GLfloat last_time = glfwGetTime();

    while (!main_window->should_be_closed())
    {
        GLfloat now = glfwGetTime();
        GLfloat dt = now - last_time;
        last_time = now;

        // Get and handle user input events
        glfwPollEvents();

        camera.handle_keys(main_window->get_keys());
        camera.handle_mouse(main_window->get_x_change(), main_window->get_y_change());
        camera.update(dt);

        // Runtime: move the ceiling light with arrow keys and page up/down
        const auto &keys = main_window->get_keys();
        glm::vec3 lp = ceilingLight.get_position();
        const float lightSpeed = 3.0f; // units per second
        if (keys[GLFW_KEY_UP])
            lp.z -= lightSpeed * dt;
        if (keys[GLFW_KEY_DOWN])
            lp.z += lightSpeed * dt;
        if (keys[GLFW_KEY_LEFT])
            lp.x -= lightSpeed * dt;
        if (keys[GLFW_KEY_RIGHT])
            lp.x += lightSpeed * dt;
        // Use ',' and '.' to move light up/down: ',' lowers, '.' raises
        if (keys[GLFW_KEY_PERIOD])
            lp.y += lightSpeed * dt;
        if (keys[GLFW_KEY_COMMA])
            lp.y -= lightSpeed * dt;
        // commit new position if changed
        static glm::vec3 prevLp = lp;
        if (lp != prevLp)
        {
            ceilingLight.set_position(lp);
            // update bulb visual (we only have one bulb)
            if (!lightbulbs.empty())
                lightbulbs[0].set_position(lp);
            prevLp = lp;
            std::cout << "light position = (" << lp.x << ", " << lp.y << ", " << lp.z << ")\n";
        }

        // (Removed runtime shadow key handlers and noisy console prints)

        // --- Shadow pass for the single point light (skip if disabled) ---
        if (enableShadows)
        {
            glm::vec3 light_pos = ceilingLight.get_position();
            depthShader->use();
            float near_plane = 0.1f;
            glm::mat4 shadow_proj = glm::perspective(glm::radians(90.0f), 1.0f, near_plane, SHADOW_FAR);
            auto shadow_views = shadowCubemap.get_shadow_views(light_pos);

            // Render to each face
            glViewport(0, 0, SHADOW_SIZE, SHADOW_SIZE);
            glBindFramebuffer(GL_FRAMEBUFFER, shadowCubemap.get_fbo());
            for (unsigned int face = 0; face < 6; ++face)
            {
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, shadowCubemap.get_depth_cubemap_id(), 0);
                glClear(GL_DEPTH_BUFFER_BIT);

                // Set projection and view for this face
                glUniformMatrix4fv(depthShader->get_uniform_projection_id(), 1, GL_FALSE, glm::value_ptr(shadow_proj));
                glUniformMatrix4fv(depthShader->get_uniform_view_id(), 1, GL_FALSE, glm::value_ptr(shadow_views[face]));
                glUniform3fv(glGetUniformLocation(depthShader->get_program_id(), "lightPos"), 1, glm::value_ptr(light_pos));
                glUniform1f(glGetUniformLocation(depthShader->get_program_id(), "far_plane"), SHADOW_FAR);

                // Render scene geometry for depth
                // Rooms (render all room instances so walls/floor cast shadows)
                for (size_t ri = 0; ri < rooms.size() && ri < roomTransforms.size(); ++ri)
                {
                    rooms[ri].render(depthShader, roomTransforms[ri]);
                }
                // Imported models
                if (!imported_models.empty())
                {
                    // Render the same four instances used in the main pass so shadows
                    // are generated for each table instance.
                    const float modelScale = 2.0f;
                    const float floorY = -2.0f;
                    std::vector<glm::vec3> positions = {
                        glm::vec3(0.0f, floorY, 6.0f),
                        glm::vec3(0.0f, floorY, -6.0f),
                        glm::vec3(6.0f, floorY, 0.0f),
                        glm::vec3(-6.0f, floorY, 0.0f)};
                    std::vector<float> rotations = {
                        0.0f,
                        glm::pi<float>(),
                        glm::radians(-90.0f),
                        glm::radians(90.0f)};

                    for (auto &r : imported_models)
                    {
                        for (size_t i = 0; i < positions.size(); ++i)
                        {
                            glm::mat4 modelMat{1.0f};
                            modelMat = glm::translate(modelMat, positions[i]);
                            modelMat = glm::rotate(modelMat, rotations[i], glm::vec3(0.0f, 1.0f, 0.0f));
                            modelMat = glm::scale(modelMat, glm::vec3(modelScale));
                            modelMat = modelMat * r.transform;
                            glUniformMatrix4fv(depthShader->get_uniform_model_id(), 1, GL_FALSE, glm::value_ptr(modelMat));
                            r.mesh->render();
                        }
                    }

                    // Also render the props into the depth cubemap so they cast shadows.
                    // Compute table height from imported_models (max of src heights)
                    float tableHeight = 0.0f;
                    for (auto &r : imported_models)
                    {
                        tableHeight = std::max(tableHeight, r.src_max.y - r.src_min.y);
                    }
                    tableHeight *= modelScale;

                    // Target tabletop footprint (meters) used to scale props to a reasonable size.
                    // per-prop target footprint (meters): [spray, bust, duck, rat]
                    std::vector<float> perPropFootprint = {0.15f, 0.6f, 0.8f, 1.5f};
                    for (size_t i = 0; i < prop_models.size() && i < positions.size(); ++i)
                    {
                        auto &group = prop_models[i];
                        glm::vec3 basePos = positions[i];
                        // place prop bottom on the table top
                        basePos.y = floorY + tableHeight + 0.02f; // small epsilon above table
                        for (auto &pr : group)
                        {
                            // compute source bbox and center

                            glm::vec3 srcSize = pr.src_max - pr.src_min;
                            glm::vec3 srcCenter = (pr.src_min + pr.src_max) * 0.5f;
                            // use X and Z for footprint; allow per-prop footprint tuning
                            float footprintDim = std::max(0.001f, std::max(srcSize.x, srcSize.z));
                            float target = (i < perPropFootprint.size()) ? perPropFootprint[i] : 0.6f;
                            float scaleUniform = target / footprintDim;
                            scaleUniform = std::clamp(scaleUniform, 0.02f, 10.0f);

                            // Build model matrix: translate to table top, scale, apply loader transform and center horizontally
                            glm::mat4 modelMat{1.0f};
                            modelMat = glm::translate(modelMat, basePos);
                            modelMat = glm::scale(modelMat, glm::vec3(scaleUniform));
                            // apply loader's transform (aligns minY to 0). Center only X/Z (avoid changing Y placement due to centering)
                            glm::vec3 centerXZ = glm::vec3(srcCenter.x, 0.0f, srcCenter.z);
                            modelMat = modelMat * pr.transform * glm::translate(glm::mat4(1.0f), -centerXZ);

                            glUniformMatrix4fv(depthShader->get_uniform_model_id(), 1, GL_FALSE, glm::value_ptr(modelMat));
                            pr.mesh->render();
                        }
                    }
                }
            }
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            // Restore viewport to window
            glViewport(0, 0, main_window->get_buffer_width(), main_window->get_buffer_height());
        }

        // Clear the window
        glClearColor(0.f, 0.f, 0.f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // --- Render the room ---
        Data::shader_list[0]->use();

        glUniform1i(Data::shader_list[0]->get_uniform_texture_sampler_id(), 0);
        glUniform1i(glGetUniformLocation(Data::shader_list[0]->get_program_id(), "normal_sampler"), 1);
        // Bind shadow cubemap to texture unit 3 and pass far plane
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_CUBE_MAP, shadowCubemap.get_depth_cubemap_id());
        glUniform1i(glGetUniformLocation(Data::shader_list[0]->get_program_id(), "shadowMap"), 3);
        // Inform shader whether shadows are enabled
        glUniform1i(glGetUniformLocation(Data::shader_list[0]->get_program_id(), "enableShadows"), enableShadows ? 1 : 0);
        glUniform1f(glGetUniformLocation(Data::shader_list[0]->get_program_id(), "far_plane"), SHADOW_FAR);
        // PCF sampling radius (world units). Adjust to taste: larger -> softer shadows
        glUniform1f(glGetUniformLocation(Data::shader_list[0]->get_program_id(), "shadowRadius"), 0.12f);

        // Set projection and view matrices
        glUniformMatrix4fv(Data::shader_list[0]->get_uniform_projection_id(), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(Data::shader_list[0]->get_uniform_view_id(), 1, GL_FALSE, glm::value_ptr(camera.get_view_matrix()));

        // Set lighting and camera position uniforms
        glUniform3fv(glGetUniformLocation(Data::shader_list[0]->get_program_id(), "viewPosition"), 1, glm::value_ptr(camera.get_position()));

        // --- Set Light Uniforms ---
        // Directional light
        glUniform3f(glGetUniformLocation(Data::shader_list[0]->get_program_id(), "dirLight.direction"), -0.2f, -1.0f, -0.3f);
        glUniform3f(glGetUniformLocation(Data::shader_list[0]->get_program_id(), "dirLight.diffuse"), 0.5f, 0.5f, 0.5f);
        glUniform3f(glGetUniformLocation(Data::shader_list[0]->get_program_id(), "dirLight.specular"), 0.1f, 0.1f, 0.1f);

        // Point lights
        for (size_t i = 0; i < lightbulbs.size(); ++i)
        {
            lightbulbs[i].use_light(Data::shader_list[0], i);
        }

        // Render the room instances (museum composed of multiple rooms)
        for (size_t i = 0; i < roomTransforms.size() && i < rooms.size(); ++i)
        {
            rooms[i].render(Data::shader_list[0], roomTransforms[i]);
        }

        // --- Render imported model(s) (simple test) ---
        if (!imported_models.empty())
        {
            Data::shader_list[0]->use();
            // Ensure shader texture bindings
            glUniform1i(Data::shader_list[0]->get_uniform_texture_sampler_id(), 0);
            glUniform1i(glGetUniformLocation(Data::shader_list[0]->get_program_id(), "normal_sampler"), 1);

            // Render multiple instances of the imported model (4 tables, one near each wall)
            glUniform1f(glGetUniformLocation(Data::shader_list[0]->get_program_id(), "material.shininess"), 32.0f);

            // Base transform: scale and move the model to sit on the room floor at y = -2.0
            // Note: r.transform already translates mesh so its local min Y == 0.0
            const float modelScale = 2.0f;
            const float floorY = -2.0f;

            // Positions for four tables (centered, near each wall)
            std::vector<glm::vec3> positions = {
                glm::vec3(0.0f, floorY, 6.0f),  // front (towards +Z)
                glm::vec3(0.0f, floorY, -6.0f), // back (towards -Z)
                glm::vec3(6.0f, floorY, 0.0f),  // right (towards +X)
                glm::vec3(-6.0f, floorY, 0.0f)  // left (towards -X)
            };

            std::vector<float> rotations = {
                0.0f,                 // front: no rotation
                glm::pi<float>(),     // back: 180 degrees
                glm::radians(-90.0f), // right: rotate -90 around Y
                glm::radians(90.0f)   // left: rotate 90 around Y
            };

            for (auto &r : imported_models)
            {
                for (size_t i = 0; i < positions.size(); ++i)
                {
                    glm::mat4 modelMat{1.0f};
                    modelMat = glm::translate(modelMat, positions[i]);
                    modelMat = glm::rotate(modelMat, rotations[i], glm::vec3(0.0f, 1.0f, 0.0f));
                    modelMat = glm::scale(modelMat, glm::vec3(modelScale));
                    // apply the loader's local transform (which aligned mesh bottom to 0)
                    modelMat = modelMat * r.transform;

                    glUniformMatrix4fv(Data::shader_list[0]->get_uniform_model_id(), 1, GL_FALSE, glm::value_ptr(modelMat));

                    // Bind the model's textures (or fallbacks created by the loader)
                    if (r.albedo)
                        r.albedo->use();
                    else
                        fallback_albedo->use();

                    glActiveTexture(GL_TEXTURE1);
                    if (r.normal)
                        glBindTexture(GL_TEXTURE_2D, r.normal->get_id());
                    else
                        glBindTexture(GL_TEXTURE_2D, fallback_normal->get_id());

                    r.mesh->render();
                }
            }

            // Render props (one group per table) on top of the tables in the main pass
            // Compute table height from imported_models (max of src heights)
            float tableHeight = 0.0f;
            for (auto &r : imported_models)
            {
                tableHeight = std::max(tableHeight, r.src_max.y - r.src_min.y);
            }
            tableHeight *= modelScale;

            const float targetFootprint = 0.6f;
            // per-prop footprint matching used in the main pass as well
            std::vector<float> perPropFootprintMain = {0.15f, 0.6f, 0.8f, 1.5f};
            for (size_t i = 0; i < prop_models.size() && i < positions.size(); ++i)
            {
                auto &group = prop_models[i];
                glm::vec3 basePos = positions[i];
                basePos.y = floorY + tableHeight + 0.02f; // small epsilon above table
                for (auto &pr : group)
                {

                    glm::vec3 srcSize = pr.src_max - pr.src_min;
                    glm::vec3 srcCenter = (pr.src_min + pr.src_max) * 0.5f;
                    float footprintDim = std::max(0.001f, std::max(srcSize.x, srcSize.z));
                    float target = (i < perPropFootprintMain.size()) ? perPropFootprintMain[i] : targetFootprint;
                    float scaleUniform = target / footprintDim;
                    scaleUniform = std::clamp(scaleUniform, 0.02f, 10.0f);

                    glm::mat4 modelMat{1.0f};
                    modelMat = glm::translate(modelMat, basePos);
                    modelMat = glm::scale(modelMat, glm::vec3(scaleUniform));
                    glm::vec3 centerXZ = glm::vec3(srcCenter.x, 0.0f, srcCenter.z);
                    modelMat = modelMat * pr.transform * glm::translate(glm::mat4(1.0f), -centerXZ);

                    glUniformMatrix4fv(Data::shader_list[0]->get_uniform_model_id(), 1, GL_FALSE, glm::value_ptr(modelMat));

                    if (pr.albedo)
                        pr.albedo->use();
                    else
                        fallback_albedo->use();
                    glActiveTexture(GL_TEXTURE1);
                    if (pr.normal)
                        glBindTexture(GL_TEXTURE_2D, pr.normal->get_id());
                    else
                        glBindTexture(GL_TEXTURE_2D, fallback_normal->get_id());

                    pr.mesh->render();
                }
            }
        }

        // --- Render the lightbulbs ---
        Data::shader_list[1]->use();
        glUniformMatrix4fv(glGetUniformLocation(Data::shader_list[1]->get_program_id(), "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(Data::shader_list[1]->get_program_id(), "view"), 1, GL_FALSE, glm::value_ptr(camera.get_view_matrix()));

        for (const auto &bulb : lightbulbs)
        {
            bulb.render(Data::shader_list[1]);
        }

        glUseProgram(0);

        main_window->swap_buffers();
    }

    return EXIT_SUCCESS;
}
