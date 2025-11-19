#include <iostream>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>

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

// renderQuad() renders a 1x1 XY quad in NDC
unsigned int quadVAO = 0;
unsigned int quadVBO;
// void renderQuad()
// {
//     if (quadVAO == 0)
//     {
//         float quadVertices[] = {
//             // positions        // texture Coords
//             -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
//             -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
//              1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
//              1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
//         };
//         // setup plane VAO
//         glGenVertexArrays(1, &quadVAO);
//         glGenBuffers(1, &quadVBO);
//         glBindVertexArray(quadVAO);
//         glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
//         glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
//         glEnableVertexAttribArray(0);
//         glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
//         glEnableVertexAttribArray(1);
//         glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
//     }
//     glBindVertexArray(quadVAO);
//     glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
//     glBindVertexArray(0);
// }

int main()
{
    // Window dimensions
    constexpr GLint WIDTH = 1200;
    constexpr GLint HEIGHT = 800;
    
    // Spotlight settings
    const float spotOuterDeg = 40.0f;

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

    // Load potted plant model (one to place in each corner). If the file
    // doesn't exist or fails to load, AssimpLoader::loadModel returns empty
    // and the code below will simply skip rendering them.
    std::vector<AssimpLoader::Renderable> potted_models = AssimpLoader::loadModel(Data::root_path / "models" / "potted_plant_01_1k.gltf");
    // Load picture frames to place on the walls. If missing, loader returns empty and they are skipped.
    std::vector<AssimpLoader::Renderable> picture_models = AssimpLoader::loadModel(Data::root_path / "models" / "fancy_picture_frame_01_1k.gltf");
    // Load the second variant and place them on the opposite side of the door holes.
    std::vector<AssimpLoader::Renderable> picture2_models = AssimpLoader::loadModel(Data::root_path / "models" / "fancy_picture_frame_02_1k.gltf");

    // Load new "big statue" models
    std::vector<AssimpLoader::Renderable> cat_statue = AssimpLoader::loadModel(Data::root_path / "models" / "concrete_cat_statue_1k.gltf");
    std::vector<AssimpLoader::Renderable> cannon_statue = AssimpLoader::loadModel(Data::root_path / "models" / "cannon_01_1k.gltf");
    std::vector<AssimpLoader::Renderable> cart_statue = AssimpLoader::loadModel(Data::root_path / "models" / "CoffeeCart_01_1k.gltf");
    std::vector<AssimpLoader::Renderable> drill_statue = AssimpLoader::loadModel(Data::root_path / "models" / "Drill_01_1k.gltf");
    std::vector<AssimpLoader::Renderable> horse_statue = AssimpLoader::loadModel(Data::root_path / "models" / "horse_head_1k.gltf");
    
    // Load new potted plant for outer rooms
    std::vector<AssimpLoader::Renderable> potted_plant_02 = AssimpLoader::loadModel(Data::root_path / "models" / "potted_plant_02_1k.gltf");

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

    // --- Spot shadow maps (one per room) ---
    const int SPOT_COUNT = 5;
    const unsigned int SPOT_SHADOW_RES = 1024;
    std::vector<GLuint> spotDepthMaps(SPOT_COUNT, 0);
    std::vector<GLuint> spotDepthFBOs(SPOT_COUNT, 0);
    for (int i = 0; i < SPOT_COUNT; ++i)
    {
        glGenTextures(1, &spotDepthMaps[i]);
        glBindTexture(GL_TEXTURE_2D, spotDepthMaps[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 2048, 2048, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

        GLuint fbo = 0;
        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, spotDepthMaps[i], 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            // Spot shadow FBO not complete (no debug output)
            std::cerr << "Spot shadow FBO not complete!" << std::endl;
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        spotDepthFBOs[i] = fbo;
    }

    // Depth shader for spotlights (simple depth-only shader)
    auto spotDepthShader = Shader::create_from_files(Data::root_path / "shaders" / "spot_depth.vert", Data::root_path / "shaders" / "spot_depth.frag");
    
    // Debug shader for quad rendering
    // std::shared_ptr<Shader> debugDepthQuad = Shader::create_from_files("shaders/debug_quad.vert", "shaders/debug_quad.frag");, Data::root_path / "shaders" / "debug_quad.frag");

    // Debug mode state
    // Debug mode toggle: 0=off, 1-5=show spot shadow map 1-5
    // int debugMode = 0;
    // bool vKeyPressed = false;

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
        
        // Ensure depth testing is enabled (in case it was disabled elsewhere)
        glEnable(GL_DEPTH_TEST);

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
        }

        // Debug toggle
        // if (main_window->get_key(GLFW_KEY_V) == GLFW_PRESS)
        // {
        //     if (!vKeyPressed)
        //     {
        //         debugMode = (debugMode + 1) % (SPOT_COUNT + 1);
        //         vKeyPressed = true;
        //         std::cout << "Debug Mode: " << debugMode << std::endl;
        //     }
        // }
        // else
        // {
        //     vKeyPressed = false;
        // }

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
            // Enable face culling for the depth pass so only the side of
            // geometry facing the light contributes to the shadow map.
            // This prevents thin duplicated geometry (two-sided walls) from
            // both writing depth and causing shadows to appear on the
            // opposite face.
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
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
                // Rooms (render only shadow-casting parts so walls/floor cast
                // shadows but duplicated back-faces do not write depth).
                for (size_t ri = 0; ri < rooms.size() && ri < roomTransforms.size(); ++ri)
                {
                    rooms[ri].render_for_depth(depthShader, roomTransforms[ri]);
                }
                // Imported models
                if (!imported_models.empty())
                {
                    // Render the same four instances used in the main pass so shadows
                    // are generated for each table instance.
                    const float modelScale = 2.0f;
                    const float floorY = -2.0f;
                    std::vector<glm::vec3> positions = {
                        glm::vec3(0.0f, floorY, 28.5f),
                        glm::vec3(0.0f, floorY, -28.5f),
                        glm::vec3(28.5f, floorY, 0.0f),
                        glm::vec3(-28.5f, floorY, 0.0f)};
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

                    // Also render potted plants into the depth cubemap so they cast shadows
                    if (!potted_models.empty())
                    {
                        // positions for four corners (near each corner of the central room)
                        std::vector<glm::vec3> potPositions = {
                            glm::vec3(8.0f, floorY, 8.0f),
                            glm::vec3(-8.0f, floorY, 8.0f),
                            glm::vec3(8.0f, floorY, -8.0f),
                            glm::vec3(-8.0f, floorY, -8.0f)};
                        std::vector<float> potRot = {0.0f, glm::pi<float>(), glm::radians(90.0f), glm::radians(-90.0f)};
                        const float potScale = 4.0f;

                        for (auto &pr : potted_models)
                        {
                            for (size_t i = 0; i < potPositions.size(); ++i)
                            {
                                glm::mat4 modelMat{1.0f};
                                modelMat = glm::translate(modelMat, potPositions[i]);
                                modelMat = glm::rotate(modelMat, potRot[i], glm::vec3(0.0f, 1.0f, 0.0f));
                                modelMat = glm::scale(modelMat, glm::vec3(potScale));
                                modelMat = modelMat * pr.transform;
                                glUniformMatrix4fv(depthShader->get_uniform_model_id(), 1, GL_FALSE, glm::value_ptr(modelMat));
                                pr.mesh->render();
                            }
                        }
                    }

                    // Also render picture frames into the depth cubemap (one per wall)
                    if (!picture_models.empty())
                    {
                        const float pictureYOffset = 3.5f; // meters above floor to center picture
                        const float pictureShift = 4.0f;   // slightly larger rightward shift
                        const float wallDist = 9.8f;       // place pictures closer to wall (walls at +/-10)
                        std::vector<glm::vec3> picPositions = {
                            // front/back: shift along +X (right)
                            glm::vec3(pictureShift, floorY + pictureYOffset, wallDist),   // front (moved right, closer to wall)
                            glm::vec3(pictureShift, floorY + pictureYOffset, -wallDist),  // back (moved right, closer to wall)
                            // left/right walls: X at wallDist, shift along +Z
                            glm::vec3(wallDist, floorY + pictureYOffset, pictureShift),   // right wall (moved towards +Z)
                            glm::vec3(-wallDist, floorY + pictureYOffset, pictureShift)   // left wall (moved towards +Z)
                        };
                        // Ensure pictures face into the room: fix rotations so no backside shows
                        std::vector<float> picRot = {glm::pi<float>(), 0.0f, glm::radians(270.0f), glm::radians(90.0f)};
                        const float picScale = 4.0f; // keep large scale
                        for (auto &pr : picture_models)
                        {
                            for (size_t i = 0; i < picPositions.size(); ++i)
                            {
                                glm::mat4 modelMat{1.0f};
                                modelMat = glm::translate(modelMat, picPositions[i]);
                                modelMat = glm::rotate(modelMat, picRot[i], glm::vec3(0.0f, 1.0f, 0.0f));
                                modelMat = glm::scale(modelMat, glm::vec3(picScale));
                                modelMat = modelMat * pr.transform;
                                glUniformMatrix4fv(depthShader->get_uniform_model_id(), 1, GL_FALSE, glm::value_ptr(modelMat));
                                pr.mesh->render();
                            }
                        }
                        // Render the second picture-frame variant on the opposite side of each door
                        if (!picture2_models.empty())
                        {
                            std::vector<glm::vec3> pic2Positions = {
                                glm::vec3(-pictureShift, floorY + pictureYOffset, wallDist),
                                glm::vec3(-pictureShift, floorY + pictureYOffset, -wallDist),
                                glm::vec3(wallDist, floorY + pictureYOffset, -pictureShift),
                                glm::vec3(-wallDist, floorY + pictureYOffset, -pictureShift)};
                            for (auto &pr2 : picture2_models)
                            {
                                for (size_t i = 0; i < pic2Positions.size(); ++i)
                                {
                                    glm::mat4 modelMat{1.0f};
                                    modelMat = glm::translate(modelMat, pic2Positions[i]);
                                    // reuse same rotation and scale as the first variant so they match
                                    modelMat = glm::rotate(modelMat, picRot[i], glm::vec3(0.0f, 1.0f, 0.0f));
                                    modelMat = glm::scale(modelMat, glm::vec3(picScale));
                                    modelMat = modelMat * pr2.transform;
                                    glUniformMatrix4fv(depthShader->get_uniform_model_id(), 1, GL_FALSE, glm::value_ptr(modelMat));
                                    pr2.mesh->render();
                                }
                            }
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

                // Render new "big statues" in the point shadow pass
                const float defaultStatueScale = 12.0f;
                const float cannonScale = 3.0f;
                const float coffeeScale = 2.0f; 
                const float floorY = -2.0f;
                
                // Cat (Center)
                if (!cat_statue.empty()) {
                    glm::mat4 modelMat{1.0f};
                    modelMat = glm::translate(modelMat, glm::vec3(0.0f, floorY, 0.0f));
                    modelMat = glm::scale(modelMat, glm::vec3(defaultStatueScale));
                    for (auto &r : cat_statue) {
                        glm::mat4 m = modelMat * r.transform;
                        glUniformMatrix4fv(depthShader->get_uniform_model_id(), 1, GL_FALSE, glm::value_ptr(m));
                        r.mesh->render();
                    }
                }
                // Cannon (+X)
                if (!cannon_statue.empty()) {
                    glm::mat4 modelMat{1.0f};
                    modelMat = glm::translate(modelMat, glm::vec3(20.0f, floorY, 0.0f));
                    modelMat = glm::scale(modelMat, glm::vec3(cannonScale));
                    for (auto &r : cannon_statue) {
                        glm::mat4 m = modelMat * r.transform;
                        glUniformMatrix4fv(depthShader->get_uniform_model_id(), 1, GL_FALSE, glm::value_ptr(m));
                        r.mesh->render();
                    }
                }
                // Cart (-X)
                if (!cart_statue.empty()) {
                    glm::mat4 modelMat{1.0f};
                    modelMat = glm::translate(modelMat, glm::vec3(-20.0f, floorY, 0.0f));
                    modelMat = glm::scale(modelMat, glm::vec3(coffeeScale));
                    for (auto &r : cart_statue) {
                        glm::mat4 m = modelMat * r.transform;
                        glUniformMatrix4fv(depthShader->get_uniform_model_id(), 1, GL_FALSE, glm::value_ptr(m));
                        r.mesh->render();
                    }
                }
                // Drill (+Z)
                if (!drill_statue.empty()) {
                    glm::mat4 modelMat{1.0f};
                    modelMat = glm::translate(modelMat, glm::vec3(0.0f, floorY, 20.0f));
                    modelMat = glm::scale(modelMat, glm::vec3(defaultStatueScale));
                    for (auto &r : drill_statue) {
                        glm::mat4 m = modelMat * r.transform;
                        glUniformMatrix4fv(depthShader->get_uniform_model_id(), 1, GL_FALSE, glm::value_ptr(m));
                        r.mesh->render();
                    }
                }
                // Horse (-Z)
                if (!horse_statue.empty()) {
                    glm::mat4 modelMat{1.0f};
                    modelMat = glm::translate(modelMat, glm::vec3(0.0f, floorY, -20.0f));
                    modelMat = glm::scale(modelMat, glm::vec3(defaultStatueScale));
                    for (auto &r : horse_statue) {
                        glm::mat4 m = modelMat * r.transform;
                        glUniformMatrix4fv(depthShader->get_uniform_model_id(), 1, GL_FALSE, glm::value_ptr(m));
                        r.mesh->render();
                    }
                }

                // Render new potted plants in outer rooms (Point Shadow Pass)
                if (!potted_plant_02.empty()) {
                    const float potScale = 4.0f;
                    std::vector<glm::vec3> outerRoomCenters = {
                        glm::vec3(20.0f, 0.0f, 0.0f),  // East
                        glm::vec3(-20.0f, 0.0f, 0.0f), // West
                        glm::vec3(0.0f, 0.0f, 20.0f),  // South
                        glm::vec3(0.0f, 0.0f, -20.0f)  // North
                    };
                    // Offsets for 4 corners relative to room center
                    std::vector<glm::vec3> cornerOffsets = {
                        glm::vec3(8.0f, 0.0f, 8.0f),
                        glm::vec3(-8.0f, 0.0f, 8.0f),
                        glm::vec3(8.0f, 0.0f, -8.0f),
                        glm::vec3(-8.0f, 0.0f, -8.0f)
                    };

                    for (const auto& center : outerRoomCenters) {
                        for (const auto& offset : cornerOffsets) {
                            glm::vec3 pos = center + offset;
                            pos.y = floorY; // Ensure correct floor height
                            
                            glm::mat4 modelMat{1.0f};
                            modelMat = glm::translate(modelMat, pos);
                            modelMat = glm::scale(modelMat, glm::vec3(potScale));
                            for (auto &r : potted_plant_02) {
                                glm::mat4 m = modelMat * r.transform;
                                glUniformMatrix4fv(depthShader->get_uniform_model_id(), 1, GL_FALSE, glm::value_ptr(m));
                                r.mesh->render();
                            }
                        }
                    }
                }
            }
            // Restore face culling state
            glDisable(GL_CULL_FACE);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            // Restore viewport to window
            glViewport(0, 0, main_window->get_buffer_width(), main_window->get_buffer_height());
        }

        // --- Spot shadow pass: render each spotlight's depth map ---
        {
            // We'll render from each room's ceiling downward with a perspective
            // projection matching the spot outer cone.
            // projection matching the spot outer cone.
            const float spotFov = spotOuterDeg * 2.0f; // cover full cone
            const float near_plane_spot = 0.1f;
            const float far_plane_spot = 25.0f;

            for (int si = 0; si < (int)roomTransforms.size() && si < (int)spotDepthFBOs.size(); ++si)
            {
                glm::vec3 spos = glm::vec3(roomTransforms[si] * glm::vec4(0.0f, 7.5f, 0.0f, 1.0f));
                glm::vec3 sdir = glm::vec3(0.0f, -1.0f, 0.0f);

                glm::mat4 lightProj = glm::perspective(glm::radians(spotFov), 1.0f, near_plane_spot, far_plane_spot);
                // choose an up vector that is not parallel to direction
                glm::vec3 up = fabs(sdir.y) > 0.99f ? glm::vec3(0.0f, 0.0f, 1.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
                glm::mat4 lightView = glm::lookAt(spos, spos + sdir, up);
                glm::mat4 lightSpace = lightProj * lightView;

                // Render scene depth from spotlight POV
                glViewport(0, 0, 2048, 2048);
                glBindFramebuffer(GL_FRAMEBUFFER, spotDepthFBOs[si]);
                glClear(GL_DEPTH_BUFFER_BIT);
                
                // Disable face culling for the depth pass to ensure all geometry
                // (regardless of winding order) is rendered into the shadow map.
                glDisable(GL_CULL_FACE);
                // glCullFace(GL_BACK);

                spotDepthShader->use();
                glUniformMatrix4fv(glGetUniformLocation(spotDepthShader->get_program_id(), "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpace));

                // Render rooms (shadow-caster geometry)
                for (size_t ri = 0; ri < rooms.size() && ri < roomTransforms.size(); ++ri)
                {
                    rooms[ri].render_for_depth(spotDepthShader, roomTransforms[ri]);
                }

                // Render imported models and props into spot depth
                if (!imported_models.empty())
                {
                    const float modelScale = 2.0f;
                    const float floorY = -2.0f;
                    std::vector<glm::vec3> positions = {
                        glm::vec3(0.0f, floorY, 28.5f),
                        glm::vec3(0.0f, floorY, -28.5f),
                        glm::vec3(28.5f, floorY, 0.0f),
                        glm::vec3(-28.5f, floorY, 0.0f)};
                    std::vector<float> rotations = {0.0f, glm::pi<float>(), glm::radians(-90.0f), glm::radians(90.0f)};

                    for (auto &r : imported_models)
                    {
                        for (size_t i = 0; i < positions.size(); ++i)
                        {
                            glm::mat4 modelMat{1.0f};
                            modelMat = glm::translate(modelMat, positions[i]);
                            modelMat = glm::rotate(modelMat, rotations[i], glm::vec3(0.0f, 1.0f, 0.0f));
                            modelMat = glm::scale(modelMat, glm::vec3(modelScale));
                            modelMat = modelMat * r.transform;
                            glUniformMatrix4fv(spotDepthShader->get_uniform_model_id(), 1, GL_FALSE, glm::value_ptr(modelMat));
                            r.mesh->render();
                        }
                    }
                    // Also render potted plants into spot depth so they cast shadows from spotlights
                    if (!potted_models.empty())
                    {
                        std::vector<glm::vec3> potPositions = {
                            glm::vec3(8.0f, floorY, 8.0f),
                            glm::vec3(-8.0f, floorY, 8.0f),
                            glm::vec3(8.0f, floorY, -8.0f),
                            glm::vec3(-8.0f, floorY, -8.0f)};
                        std::vector<float> potRot = {0.0f, glm::pi<float>(), glm::radians(90.0f), glm::radians(-90.0f)};
                        const float potScale = 4.0f;
                        for (auto &pr : potted_models)
                        {
                            for (size_t i = 0; i < potPositions.size(); ++i)
                            {
                                glm::mat4 modelMat{1.0f};
                                modelMat = glm::translate(modelMat, potPositions[i]);
                                modelMat = glm::rotate(modelMat, potRot[i], glm::vec3(0.0f, 1.0f, 0.0f));
                                modelMat = glm::scale(modelMat, glm::vec3(potScale));
                                modelMat = modelMat * pr.transform;
                                glUniformMatrix4fv(spotDepthShader->get_uniform_model_id(), 1, GL_FALSE, glm::value_ptr(modelMat));
                                pr.mesh->render();
                            }
                        }
                    }
                    // Also render picture frames into spot depth so they cast spot-shadow contribution
                    if (!picture_models.empty())
                    {
                        const float pictureYOffset = 3.5f;
                        const float pictureShift = 4.0f;
                        const float wallDist = 9.8f;
                        std::vector<glm::vec3> picPositions = {
                            glm::vec3(pictureShift, floorY + pictureYOffset, wallDist),
                            glm::vec3(pictureShift, floorY + pictureYOffset, -wallDist),
                            glm::vec3(wallDist, floorY + pictureYOffset, pictureShift),
                            glm::vec3(-wallDist, floorY + pictureYOffset, pictureShift)};
                        std::vector<float> picRot = {glm::pi<float>(), 0.0f, glm::radians(270.0f), glm::radians(90.0f)};
                        const float picScale = 4.0f;
                        for (auto &pr : picture_models)
                        {
                            for (size_t i = 0; i < picPositions.size(); ++i)
                            {
                                glm::mat4 modelMat{1.0f};
                                modelMat = glm::translate(modelMat, picPositions[i]);
                                modelMat = glm::rotate(modelMat, picRot[i], glm::vec3(0.0f, 1.0f, 0.0f));
                                modelMat = glm::scale(modelMat, glm::vec3(picScale));
                                modelMat = modelMat * pr.transform;
                                glUniformMatrix4fv(spotDepthShader->get_uniform_model_id(), 1, GL_FALSE, glm::value_ptr(modelMat));
                                pr.mesh->render();
                            }
                        }
                        // Render the second picture-frame variant mirrored on the opposite side for spot depth
                        if (!picture2_models.empty())
                        {
                            std::vector<glm::vec3> pic2Positions = {
                                glm::vec3(-pictureShift, floorY + pictureYOffset, wallDist),
                                glm::vec3(-pictureShift, floorY + pictureYOffset, -wallDist),
                                glm::vec3(wallDist, floorY + pictureYOffset, -pictureShift),
                                glm::vec3(-wallDist, floorY + pictureYOffset, -pictureShift)};
                            for (auto &pr2 : picture2_models)
                            {
                                for (size_t i = 0; i < pic2Positions.size(); ++i)
                                {
                                    glm::mat4 modelMat{1.0f};
                                    modelMat = glm::translate(modelMat, pic2Positions[i]);
                                    modelMat = glm::rotate(modelMat, picRot[i], glm::vec3(0.0f, 1.0f, 0.0f));
                                    modelMat = glm::scale(modelMat, glm::vec3(picScale));
                                    modelMat = modelMat * pr2.transform;
                                    glUniformMatrix4fv(spotDepthShader->get_uniform_model_id(), 1, GL_FALSE, glm::value_ptr(modelMat));
                                    pr2.mesh->render();
                                }
                            }
                        }
                    }
                    // props
                    float tableHeight = 0.0f;
                    for (auto &r : imported_models)
                        tableHeight = std::max(tableHeight, r.src_max.y - r.src_min.y);
                    tableHeight *= modelScale;

                    std::vector<float> perPropFootprint = {0.15f, 0.6f, 0.8f, 1.5f};
                    for (size_t i = 0; i < prop_models.size() && i < positions.size(); ++i)
                    {
                        auto &group = prop_models[i];
                        glm::vec3 basePos = positions[i];
                        basePos.y = floorY + tableHeight + 0.02f;
                        for (auto &pr : group)
                        {
                            glm::vec3 srcSize = pr.src_max - pr.src_min;
                            glm::vec3 srcCenter = (pr.src_min + pr.src_max) * 0.5f;
                            float footprintDim = std::max(0.001f, std::max(srcSize.x, srcSize.z));
                            float target = (i < perPropFootprint.size()) ? perPropFootprint[i] : 0.6f;
                            float scaleUniform = target / footprintDim;
                            scaleUniform = std::clamp(scaleUniform, 0.02f, 10.0f);
                            glm::mat4 modelMat{1.0f};
                            modelMat = glm::translate(modelMat, basePos);
                            modelMat = glm::scale(modelMat, glm::vec3(scaleUniform));
                            glm::vec3 centerXZ = glm::vec3(srcCenter.x, 0.0f, srcCenter.z);
                            modelMat = modelMat * pr.transform * glm::translate(glm::mat4(1.0f), -centerXZ);
                            glUniformMatrix4fv(spotDepthShader->get_uniform_model_id(), 1, GL_FALSE, glm::value_ptr(modelMat));
                            pr.mesh->render();
                        }
                    }
                }





                // Render new "big statues" in the spot shadow pass
                const float defaultStatueScale = 12.0f;
                const float cannonScale = 3.0f;
                const float coffeeScale = 2.0f;
                const float floorY = -2.0f;
                
                // Cat (Center)
                if (!cat_statue.empty()) {
                    glm::mat4 modelMat{1.0f};
                    modelMat = glm::translate(modelMat, glm::vec3(0.0f, floorY, 0.0f));
                    modelMat = glm::scale(modelMat, glm::vec3(defaultStatueScale));
                    for (auto &r : cat_statue) {
                        glm::mat4 m = modelMat * r.transform;
                        glUniformMatrix4fv(spotDepthShader->get_uniform_model_id(), 1, GL_FALSE, glm::value_ptr(m));
                        r.mesh->render();
                    }
                }
                // Cannon (+X)
                if (!cannon_statue.empty()) {
                    glm::mat4 modelMat{1.0f};
                    modelMat = glm::translate(modelMat, glm::vec3(20.0f, floorY, 0.0f));
                    modelMat = glm::scale(modelMat, glm::vec3(cannonScale));
                    for (auto &r : cannon_statue) {
                        glm::mat4 m = modelMat * r.transform;
                        glUniformMatrix4fv(spotDepthShader->get_uniform_model_id(), 1, GL_FALSE, glm::value_ptr(m));
                        r.mesh->render();
                    }
                }
                // Cart (-X)
                if (!cart_statue.empty()) {
                    glm::mat4 modelMat{1.0f};
                    modelMat = glm::translate(modelMat, glm::vec3(-20.0f, floorY, 0.0f));
                    modelMat = glm::scale(modelMat, glm::vec3(coffeeScale));
                    for (auto &r : cart_statue) {
                        glm::mat4 m = modelMat * r.transform;
                        glUniformMatrix4fv(spotDepthShader->get_uniform_model_id(), 1, GL_FALSE, glm::value_ptr(m));
                        r.mesh->render();
                    }
                }
                // Drill (+Z)
                if (!drill_statue.empty()) {
                    glm::mat4 modelMat{1.0f};
                    modelMat = glm::translate(modelMat, glm::vec3(0.0f, floorY, 20.0f));
                    modelMat = glm::scale(modelMat, glm::vec3(defaultStatueScale));
                    for (auto &r : drill_statue) {
                        glm::mat4 m = modelMat * r.transform;
                        glUniformMatrix4fv(spotDepthShader->get_uniform_model_id(), 1, GL_FALSE, glm::value_ptr(m));
                        r.mesh->render();
                    }
                }
                // Horse (-Z)
                if (!horse_statue.empty()) {
                    glm::mat4 modelMat{1.0f};
                    modelMat = glm::translate(modelMat, glm::vec3(0.0f, floorY, -20.0f));
                    modelMat = glm::scale(modelMat, glm::vec3(defaultStatueScale));
                    for (auto &r : horse_statue) {
                        glm::mat4 m = modelMat * r.transform;
                        glUniformMatrix4fv(spotDepthShader->get_uniform_model_id(), 1, GL_FALSE, glm::value_ptr(m));
                        r.mesh->render();
                    }
                }

                // Render new potted plants in outer rooms (Spot Shadow Pass)
                if (!potted_plant_02.empty()) {
                    const float potScale = 4.0f;
                    std::vector<glm::vec3> outerRoomCenters = {
                        glm::vec3(20.0f, 0.0f, 0.0f),  // East
                        glm::vec3(-20.0f, 0.0f, 0.0f), // West
                        glm::vec3(0.0f, 0.0f, 20.0f),  // South
                        glm::vec3(0.0f, 0.0f, -20.0f)  // North
                    };
                    // Offsets for 4 corners relative to room center
                    std::vector<glm::vec3> cornerOffsets = {
                        glm::vec3(8.0f, 0.0f, 8.0f),
                        glm::vec3(-8.0f, 0.0f, 8.0f),
                        glm::vec3(8.0f, 0.0f, -8.0f),
                        glm::vec3(-8.0f, 0.0f, -8.0f)
                    };

                    for (const auto& center : outerRoomCenters) {
                        for (const auto& offset : cornerOffsets) {
                            glm::vec3 pos = center + offset;
                            pos.y = floorY; // Ensure correct floor height
                            
                            glm::mat4 modelMat{1.0f};
                            modelMat = glm::translate(modelMat, pos);
                            modelMat = glm::scale(modelMat, glm::vec3(potScale));
                            for (auto &r : potted_plant_02) {
                                glm::mat4 m = modelMat * r.transform;
                                glUniformMatrix4fv(spotDepthShader->get_uniform_model_id(), 1, GL_FALSE, glm::value_ptr(m));
                                r.mesh->render();
                            }
                        }
                    }
                }

                // Restore face culling state
                glDisable(GL_CULL_FACE);
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                // restore viewport
                glViewport(0, 0, main_window->get_buffer_width(), main_window->get_buffer_height());

                // Upload lightSpace matrix into main shader uniform array
                Data::shader_list[0]->use();
                std::string name = "spotLightSpaceMatrices[" + std::to_string(si) + "]";
                glUniformMatrix4fv(glGetUniformLocation(Data::shader_list[0]->get_program_id(), name.c_str()), 1, GL_FALSE, glm::value_ptr(lightSpace));
            }
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

        // Spotlights: one ceiling fixture per room. We map up to NR_SPOT_LIGHTS
        // (shader compile-time constant) spot lights from the room transforms.
        // These are simple cone lights (no shadows) and add local ceiling lighting.
        for (size_t si = 0; si < roomTransforms.size() && si < 5; ++si)
        {
            // Position is ceiling local point (y=7.5) transformed by room matrix
            glm::vec3 spos = glm::vec3(roomTransforms[si] * glm::vec4(0.0f, 7.5f, 0.0f, 1.0f));
            glm::vec3 sdir = glm::vec3(0.0f, -1.0f, 0.0f); // straight down

            std::string base = "spotLights[" + std::to_string(si) + "]";
            glUniform3fv(glGetUniformLocation(Data::shader_list[0]->get_program_id(), (base + ".position").c_str()), 1, glm::value_ptr(spos));
            glUniform3fv(glGetUniformLocation(Data::shader_list[0]->get_program_id(), (base + ".direction").c_str()), 1, glm::value_ptr(sdir));
            // Make spotlights more visible by widening cone, increasing intensity and reducing attenuation
            glUniform1f(glGetUniformLocation(Data::shader_list[0]->get_program_id(), (base + ".cutOff").c_str()), cos(glm::radians(30.0f)));
            glUniform1f(glGetUniformLocation(Data::shader_list[0]->get_program_id(), (base + ".outerCutOff").c_str()), cos(glm::radians(spotOuterDeg)));
            glUniform1f(glGetUniformLocation(Data::shader_list[0]->get_program_id(), (base + ".constant").c_str()), 1.0f);
            // Less aggressive attenuation so the spot covers more of the room
            glUniform1f(glGetUniformLocation(Data::shader_list[0]->get_program_id(), (base + ".linear").c_str()), 0.09f);
            glUniform1f(glGetUniformLocation(Data::shader_list[0]->get_program_id(), (base + ".quadratic").c_str()), 0.032f);
            // Slightly higher ambient and much stronger diffuse to make the fixture visibly light the room
            glUniform3f(glGetUniformLocation(Data::shader_list[0]->get_program_id(), (base + ".ambient").c_str()), 0.02f, 0.02f, 0.02f);
            glUniform3f(glGetUniformLocation(Data::shader_list[0]->get_program_id(), (base + ".diffuse").c_str()), 3.0f, 3.0f, 2.7f);
            glUniform3f(glGetUniformLocation(Data::shader_list[0]->get_program_id(), (base + ".specular").c_str()), 1.0f, 1.0f, 1.0f);
        }

        // Bind spot shadow maps into texture units 4..8 for shader sampling
        for (int si = 0; si < (int)spotDepthMaps.size(); ++si)
        {
            glActiveTexture(GL_TEXTURE4 + si);
            glBindTexture(GL_TEXTURE_2D, spotDepthMaps[si]);
            std::string name = "spotShadowMaps[" + std::to_string(si) + "]";
            glUniform1i(glGetUniformLocation(Data::shader_list[0]->get_program_id(), name.c_str()), 4 + si);
        }
        // spot shadows are bound; no debug toggle to set here

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
                glm::vec3(0.0f, floorY, 28.5f),  // front (towards +Z)
                glm::vec3(0.0f, floorY, -28.5f), // back (towards -Z)
                glm::vec3(28.5f, floorY, 0.0f),  // right (towards +X)
                glm::vec3(-28.5f, floorY, 0.0f)  // left (towards -X)
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

            // Render potted plants in the main spot positions (one in each corner)
            if (!potted_models.empty())
            {
                std::vector<glm::vec3> potPositions = {
                    glm::vec3(8.0f, floorY, 8.0f),
                    glm::vec3(-8.0f, floorY, 8.0f),
                    glm::vec3(8.0f, floorY, -8.0f),
                    glm::vec3(-8.0f, floorY, -8.0f)};
                std::vector<float> potRot = {0.0f, glm::pi<float>(), glm::radians(90.0f), glm::radians(-90.0f)};
                const float potScale = 4.0f;

                for (auto &pr : potted_models)
                {
                    for (size_t i = 0; i < potPositions.size(); ++i)
                    {
                        glm::mat4 modelMat{1.0f};
                        modelMat = glm::translate(modelMat, potPositions[i]);
                        modelMat = glm::rotate(modelMat, potRot[i], glm::vec3(0.0f, 1.0f, 0.0f));
                        modelMat = glm::scale(modelMat, glm::vec3(potScale));
                        modelMat = modelMat * pr.transform;

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

            // Render picture frames in the main pass (one centered on each wall)
            if (!picture_models.empty())
            {
                const float pictureYOffset = 3.5f;
                const float pictureShift = 4.0f;
                const float wallDist = 9.8f;
                std::vector<glm::vec3> picPositions = {
                    glm::vec3(pictureShift, floorY + pictureYOffset, wallDist),
                    glm::vec3(pictureShift, floorY + pictureYOffset, -wallDist),
                    glm::vec3(wallDist, floorY + pictureYOffset, pictureShift),
                    glm::vec3(-wallDist, floorY + pictureYOffset, pictureShift)};
                std::vector<float> picRot = {glm::pi<float>(), 0.0f, glm::radians(270.0f), glm::radians(90.0f)};
                const float picScale = 4.0f;

                for (auto &pr : picture_models)
                {
                    for (size_t i = 0; i < picPositions.size(); ++i)
                    {
                        glm::mat4 modelMat{1.0f};
                        modelMat = glm::translate(modelMat, picPositions[i]);
                        modelMat = glm::rotate(modelMat, picRot[i], glm::vec3(0.0f, 1.0f, 0.0f));
                        modelMat = glm::scale(modelMat, glm::vec3(picScale));
                        modelMat = modelMat * pr.transform;

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
                // Render the second picture-frame variant mirrored on the opposite side in the main pass
                if (!picture2_models.empty())
                {
                    std::vector<glm::vec3> pic2Positions = {
                        glm::vec3(-pictureShift, floorY + pictureYOffset, wallDist),
                        glm::vec3(-pictureShift, floorY + pictureYOffset, -wallDist),
                        glm::vec3(wallDist, floorY + pictureYOffset, -pictureShift),
                        glm::vec3(-wallDist, floorY + pictureYOffset, -pictureShift)};
                    for (auto &pr2 : picture2_models)
                    {
                        for (size_t i = 0; i < pic2Positions.size(); ++i)
                        {
                            glm::mat4 modelMat{1.0f};
                            modelMat = glm::translate(modelMat, pic2Positions[i]);
                            modelMat = glm::rotate(modelMat, picRot[i], glm::vec3(0.0f, 1.0f, 0.0f));
                            modelMat = glm::scale(modelMat, glm::vec3(picScale));
                            modelMat = modelMat * pr2.transform;

                            glUniformMatrix4fv(Data::shader_list[0]->get_uniform_model_id(), 1, GL_FALSE, glm::value_ptr(modelMat));

                            if (pr2.albedo)
                                pr2.albedo->use();
                            else
                                fallback_albedo->use();

                            glActiveTexture(GL_TEXTURE1);
                            if (pr2.normal)
                                glBindTexture(GL_TEXTURE_2D, pr2.normal->get_id());
                            else
                                glBindTexture(GL_TEXTURE_2D, fallback_normal->get_id());

                            pr2.mesh->render();
                        }
                    }
                }
            }

            // Render new "big statues" in the main pass
            const float defaultStatueScale = 12.0f;
            const float cannonScale = 3.0f;
            const float coffeeScale = 2.0f;
            
            // Cat (Center)
            if (!cat_statue.empty()) {
                glm::mat4 modelMat{1.0f};
                modelMat = glm::translate(modelMat, glm::vec3(0.0f, floorY, 0.0f));
                modelMat = glm::scale(modelMat, glm::vec3(defaultStatueScale));
                for (auto &r : cat_statue) {
                    glm::mat4 m = modelMat * r.transform;
                    glUniformMatrix4fv(Data::shader_list[0]->get_uniform_model_id(), 1, GL_FALSE, glm::value_ptr(m));
                    if (r.albedo) r.albedo->use(); else fallback_albedo->use();
                    glActiveTexture(GL_TEXTURE1);
                    if (r.normal) glBindTexture(GL_TEXTURE_2D, r.normal->get_id()); else glBindTexture(GL_TEXTURE_2D, fallback_normal->get_id());
                    r.mesh->render();
                }
            }
            // Cannon (+X)
            if (!cannon_statue.empty()) {
                glm::mat4 modelMat{1.0f};
                modelMat = glm::translate(modelMat, glm::vec3(20.0f, floorY, 0.0f));
                modelMat = glm::scale(modelMat, glm::vec3(cannonScale));
                for (auto &r : cannon_statue) {
                    glm::mat4 m = modelMat * r.transform;
                    glUniformMatrix4fv(Data::shader_list[0]->get_uniform_model_id(), 1, GL_FALSE, glm::value_ptr(m));
                    if (r.albedo) r.albedo->use(); else fallback_albedo->use();
                    glActiveTexture(GL_TEXTURE1);
                    if (r.normal) glBindTexture(GL_TEXTURE_2D, r.normal->get_id()); else glBindTexture(GL_TEXTURE_2D, fallback_normal->get_id());
                    r.mesh->render();
                }
            }
            // Cart (-X)
            if (!cart_statue.empty()) {
                glm::mat4 modelMat{1.0f};
                modelMat = glm::translate(modelMat, glm::vec3(-20.0f, floorY, 0.0f));
                modelMat = glm::scale(modelMat, glm::vec3(coffeeScale));
                for (auto &r : cart_statue) {
                    glm::mat4 m = modelMat * r.transform;
                    glUniformMatrix4fv(Data::shader_list[0]->get_uniform_model_id(), 1, GL_FALSE, glm::value_ptr(m));
                    if (r.albedo) r.albedo->use(); else fallback_albedo->use();
                    glActiveTexture(GL_TEXTURE1);
                    if (r.normal) glBindTexture(GL_TEXTURE_2D, r.normal->get_id()); else glBindTexture(GL_TEXTURE_2D, fallback_normal->get_id());
                    r.mesh->render();
                }
            }
            // Drill (+Z)
            if (!drill_statue.empty()) {
                glm::mat4 modelMat{1.0f};
                modelMat = glm::translate(modelMat, glm::vec3(0.0f, floorY, 20.0f));
                modelMat = glm::scale(modelMat, glm::vec3(defaultStatueScale));
                for (auto &r : drill_statue) {
                    glm::mat4 m = modelMat * r.transform;
                    glUniformMatrix4fv(Data::shader_list[0]->get_uniform_model_id(), 1, GL_FALSE, glm::value_ptr(m));
                    if (r.albedo) r.albedo->use(); else fallback_albedo->use();
                    glActiveTexture(GL_TEXTURE1);
                    if (r.normal) glBindTexture(GL_TEXTURE_2D, r.normal->get_id()); else glBindTexture(GL_TEXTURE_2D, fallback_normal->get_id());
                    r.mesh->render();
                }
            }
            // Horse (-Z)
            if (!horse_statue.empty()) {
                glm::mat4 modelMat{1.0f};
                modelMat = glm::translate(modelMat, glm::vec3(0.0f, floorY, -20.0f));
                modelMat = glm::scale(modelMat, glm::vec3(defaultStatueScale));
                for (auto &r : horse_statue) {
                    glm::mat4 m = modelMat * r.transform;
                    glUniformMatrix4fv(Data::shader_list[0]->get_uniform_model_id(), 1, GL_FALSE, glm::value_ptr(m));
                    if (r.albedo) r.albedo->use(); else fallback_albedo->use();
                    glActiveTexture(GL_TEXTURE1);
                    if (r.normal) glBindTexture(GL_TEXTURE_2D, r.normal->get_id()); else glBindTexture(GL_TEXTURE_2D, fallback_normal->get_id());
                    r.mesh->render();
                }
            }

            // Render new potted plants in outer rooms (Main Pass)
            if (!potted_plant_02.empty()) {
                const float potScale = 4.0f;
                std::vector<glm::vec3> outerRoomCenters = {
                    glm::vec3(20.0f, 0.0f, 0.0f),  // East
                    glm::vec3(-20.0f, 0.0f, 0.0f), // West
                    glm::vec3(0.0f, 0.0f, 20.0f),  // South
                    glm::vec3(0.0f, 0.0f, -20.0f)  // North
                };
                // Offsets for 4 corners relative to room center
                std::vector<glm::vec3> cornerOffsets = {
                    glm::vec3(8.0f, 0.0f, 8.0f),
                    glm::vec3(-8.0f, 0.0f, 8.0f),
                    glm::vec3(8.0f, 0.0f, -8.0f),
                    glm::vec3(-8.0f, 0.0f, -8.0f)
                };

                for (const auto& center : outerRoomCenters) {
                    for (const auto& offset : cornerOffsets) {
                        glm::vec3 pos = center + offset;
                        pos.y = floorY; // Ensure correct floor height
                        
                        glm::mat4 modelMat{1.0f};
                        modelMat = glm::translate(modelMat, pos);
                        modelMat = glm::scale(modelMat, glm::vec3(potScale));
                        for (auto &r : potted_plant_02) {
                            glm::mat4 m = modelMat * r.transform;
                            glUniformMatrix4fv(Data::shader_list[0]->get_uniform_model_id(), 1, GL_FALSE, glm::value_ptr(m));
                            if (r.albedo) r.albedo->use(); else fallback_albedo->use();
                            glActiveTexture(GL_TEXTURE1);
                            if (r.normal) glBindTexture(GL_TEXTURE_2D, r.normal->get_id()); else glBindTexture(GL_TEXTURE_2D, fallback_normal->get_id());
                            r.mesh->render();
                        }
                    }
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


        
        // Render debug quad on top if active
        // if (debugMode > 0)
        // {
        //      debugDepthQuad->use();
        //      glUniform1f(glGetUniformLocation(debugDepthQuad->get_program_id(), "near_plane"), 0.1f);
        //      glUniform1f(glGetUniformLocation(debugDepthQuad->get_program_id(), "far_plane"), 25.0f); // spot far plane
        //      glActiveTexture(GL_TEXTURE0);
        //      glBindTexture(GL_TEXTURE_2D, spotDepthMaps[debugMode - 1]);
        //      renderQuad();
        // }

        main_window->swap_buffers();
    }

    return EXIT_SUCCESS;
}
