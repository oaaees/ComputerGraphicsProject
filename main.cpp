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
#include <SkyBox.hpp>
#include <Lightbulb.hpp>
#include <AssimpLoader.hpp>
#include <Frustum.hpp>
#include <Texture.hpp>
#include <ShadowCubemap.hpp>

namespace fs = std::filesystem;

// ========== ESTRUCTURA DATA ORIGINAL (COMPLETA) ==========
struct Data
{
    static std::shared_ptr<SkyBox> sky_box;
    static std::vector<std::shared_ptr<Shader>> shader_list;
    static const fs::path root_path;
    static const fs::path vertex_shader_path;
    static const fs::path lightbulb_vertex_shader_path;
    static const fs::path fragment_shader_path;
    static const fs::path lightbulb_fragment_shader_path;
    static std::shared_ptr<Mesh> exterior_floor_mesh;
    static std::shared_ptr<Texture> exterior_floor_texture;
    static std::shared_ptr<Texture> exterior_floor_normal_texture;
    static bool exterior_floor_initialized;
};

std::vector<std::shared_ptr<Shader>> Data::shader_list{};
std::shared_ptr<SkyBox> Data::sky_box{nullptr};
std::shared_ptr<Mesh> Data::exterior_floor_mesh{nullptr};
std::shared_ptr<Texture> Data::exterior_floor_texture{nullptr};
std::shared_ptr<Texture> Data::exterior_floor_normal_texture{nullptr};
bool Data::exterior_floor_initialized = false;

const fs::path Data::root_path{fs::path{__FILE__}.parent_path()};
const fs::path Data::vertex_shader_path{Data::root_path / "shaders" / "shader.vert"};
const fs::path Data::lightbulb_vertex_shader_path{Data::root_path / "shaders" / "lightbulb.vert"};
const fs::path Data::fragment_shader_path{Data::root_path / "shaders" / "shader.frag"};
const fs::path Data::lightbulb_fragment_shader_path{Data::root_path / "shaders" / "lightbulb.frag"};

// ========== FUNCIONES AUXILIARES ORIGINALES ==========
void create_shaders_program() noexcept
{
    Data::shader_list.push_back(Shader::create_from_files(Data::vertex_shader_path, Data::fragment_shader_path));
    Data::shader_list.push_back(Shader::create_from_files(Data::lightbulb_vertex_shader_path, Data::lightbulb_fragment_shader_path));
}

void UIResponsiveWhileLoading(std::shared_ptr<Window> window) noexcept
{
    glfwPollEvents();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    window->swap_buffers();
}

void placeModelInPosition(std::vector<AssimpLoader::Renderable> model, glm::vec3 vec3, bool cullingEnabled, Frustum frustum, float radius, float scale, std::shared_ptr<Texture> fallback_albedo, std::shared_ptr<Texture> fallback_normal) noexcept
{
    glm::vec3 statuePos = vec3;
    if (!cullingEnabled || frustum.isSphereInFrustum(statuePos, radius))
    {
        glm::mat4 modelMat{1.0f};
        modelMat = glm::translate(modelMat, statuePos);
        modelMat = glm::scale(modelMat, glm::vec3(scale));
        for (auto &r : model)
        {
            glm::mat4 m = modelMat * r.transform;
            glUniformMatrix4fv(Data::shader_list[0]->get_uniform_model_id(), 1, GL_FALSE, glm::value_ptr(m));
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
}

void placeModelInPosition(std::vector<AssimpLoader::Renderable> models, std::vector<glm::vec3> positions, std::vector<float> rot, bool cullingEnabled, Frustum frustum, float radius, float scale, std::shared_ptr<Texture> fallback_albedo, std::shared_ptr<Texture> fallback_normal) noexcept
{
    for (auto &pr : models)
    {
        for (size_t i = 0; i < positions.size(); ++i)
        {
            if (!cullingEnabled || frustum.isSphereInFrustum(positions[i], radius))
            {
                glm::mat4 modelMat{1.0f};
                modelMat = glm::translate(modelMat, positions[i]);
                modelMat = glm::rotate(modelMat, rot[i], glm::vec3(0.0f, 1.0f, 0.0f));
                modelMat = glm::scale(modelMat, glm::vec3(scale));
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
}

void renderModelSpotShadow(std::vector<AssimpLoader::Renderable> model, glm::vec3 vec3, glm::vec3 spos, float far_plane_spot, bool cullingEnabled, Frustum frustum, float radius, float scale, std::shared_ptr<Shader> spotDepthShader) noexcept
{
    glm::vec3 statuePos = vec3;
    if (glm::length(statuePos - spos) > far_plane_spot + 5.0f)
    {
        // too far
    }
    else if (!cullingEnabled || frustum.isSphereInFrustum(statuePos, radius))
    {
        glm::mat4 modelMat{1.0f};
        modelMat = glm::translate(modelMat, statuePos);
        modelMat = glm::scale(modelMat, glm::vec3(scale));
        for (auto &r : model)
        {
            glm::mat4 m = modelMat * r.transform;
            glUniformMatrix4fv(spotDepthShader->get_uniform_model_id(), 1, GL_FALSE, glm::value_ptr(m));
            r.mesh->render();
        }
    }
}

void renderModelPointShadow(std::vector<AssimpLoader::Renderable> model, glm::vec3 vec3, bool cullingEnabled, Frustum frustum, float radius, float scale, std::shared_ptr<Shader> shader) noexcept
{
    glm::vec3 statuePos = vec3;
    if (!cullingEnabled || frustum.isSphereInFrustum(statuePos, radius))
    {
        glm::mat4 modelMat{1.0f};
        modelMat = glm::translate(modelMat, statuePos);
        modelMat = glm::scale(modelMat, glm::vec3(scale));
        for (auto &r : model)
        {
            glm::mat4 m = modelMat * r.transform;
            glUniformMatrix4fv(shader->get_uniform_model_id(), 1, GL_FALSE, glm::value_ptr(m));
            r.mesh->render();
        }
    }
}

void renderModelSpotShadow(std::vector<AssimpLoader::Renderable> models, std::vector<glm::vec3> positions, std::vector<float> rot, bool cullingEnabled, Frustum frustum, float radius, float scale, std::shared_ptr<Shader> shader) noexcept
{
    for (auto &pr : models)
    {
        for (size_t i = 0; i < positions.size(); ++i)
        {
            if (!cullingEnabled || frustum.isSphereInFrustum(positions[i], radius))
            {
                glm::mat4 modelMat{1.0f};
                modelMat = glm::translate(modelMat, positions[i]);
                modelMat = glm::rotate(modelMat, rot[i], glm::vec3(0.0f, 1.0f, 0.0f));
                modelMat = glm::scale(modelMat, glm::vec3(scale));
                modelMat = modelMat * pr.transform;
                glUniformMatrix4fv(shader->get_uniform_model_id(), 1, GL_FALSE, glm::value_ptr(modelMat));
                pr.mesh->render();
            }
        }
    }
}

// ========== PISO EXTERIOR ORIGINAL ==========
void initialize_exterior_floor()
{
    if (Data::exterior_floor_mesh != nullptr)
        return;

    // Crear vértices para un piso grande (200x200 unidades)
    std::vector<GLfloat> floor_vertices = {
        // x      y     z       nx   ny   nz    u     v
        -50.f, -2.1f, -50.f, 0.f, 1.f, 0.f, 0.f, 0.f,
        50.f, -2.1f, -50.f, 0.f, 1.f, 0.f, 5.f, 0.f,
        50.f, -2.1f, 50.f, 0.f, 1.f, 0.f, 5.f, 5.f,
        -50.f, -2.1f, 50.f, 0.f, 1.f, 0.f, 0.f, 5.f
    };

    std::vector<unsigned int> floor_indices = {
        0, 1, 2,
        2, 3, 0
    };

    Data::exterior_floor_mesh = Mesh::create(floor_vertices, floor_indices);

    Data::exterior_floor_texture = std::make_shared<Texture>(Data::root_path / "textures" / "grass_albedo.jpg");
    Data::exterior_floor_texture->load();
    if (!Data::exterior_floor_texture->get_id())
    {
        Data::exterior_floor_texture = std::make_shared<Texture>(150, 150, 150, 255);
        Data::exterior_floor_texture->load();
    }

    Data::exterior_floor_normal_texture = std::make_shared<Texture>(Data::root_path / "textures" / "grass_normal.png");
    Data::exterior_floor_normal_texture->load();
    if (!Data::exterior_floor_normal_texture->get_id())
    {
        Data::exterior_floor_normal_texture = std::make_shared<Texture>(128, 128, 255, 255);
        Data::exterior_floor_normal_texture->load();
    }

    Data::exterior_floor_initialized = true;
}

void render_exterior_floor(const glm::mat4 &view, const glm::mat4 &projection, const glm::vec3 &position)
{
    if (!Data::exterior_floor_initialized)
        return;

    Data::shader_list[0]->use();

    // Configurar matrices
    glUniformMatrix4fv(Data::shader_list[0]->get_uniform_view_id(), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(Data::shader_list[0]->get_uniform_projection_id(), 1, GL_FALSE, glm::value_ptr(projection));

    // Matriz de modelo
    glm::mat4 model{1.0f};
    glUniformMatrix4fv(Data::shader_list[0]->get_uniform_model_id(), 1, GL_FALSE, glm::value_ptr(model));

    // Configurar texturas
    glUniform1i(Data::shader_list[0]->get_uniform_texture_sampler_id(), 0);
    glUniform1i(glGetUniformLocation(Data::shader_list[0]->get_program_id(), "normal_sampler"), 1);

    // DESACTIVAR SOMBRAS para el piso exterior
    glUniform1i(glGetUniformLocation(Data::shader_list[0]->get_program_id(), "enableShadows"), 0);

    // LUZ DIRECCIONAL FUERTE (como sol exterior)
    glUniform3f(glGetUniformLocation(Data::shader_list[0]->get_program_id(), "dirLight.direction"), -1.0f, -0.5f, -0.5f);
    glUniform3f(glGetUniformLocation(Data::shader_list[0]->get_program_id(), "dirLight.diffuse"), 1.5f, 1.5f, 1.3f);
    glUniform3f(glGetUniformLocation(Data::shader_list[0]->get_program_id(), "dirLight.specular"), 0.2f, 0.2f, 0.2f);

    // Usar textura del piso (Unit 0)
    Data::exterior_floor_texture->use();
    glUniform1i(Data::shader_list[0]->get_uniform_texture_sampler_id(), 0);

    // Usar normal map (Unit 1)
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, Data::exterior_floor_normal_texture->get_id());
    glUniform1i(glGetUniformLocation(Data::shader_list[0]->get_program_id(), "normal_sampler"), 1);

    // Configurar material para césped
    glUniform1f(glGetUniformLocation(Data::shader_list[0]->get_program_id(), "material.shininess"), 16.0f);

    // Posición de cámara
    glUniform3fv(glGetUniformLocation(Data::shader_list[0]->get_program_id(), "viewPosition"),
                 1, glm::value_ptr(position));

    // Renderizar
    Data::exterior_floor_mesh->render();
}

// ========== CONFIGURACIONES ==========
struct SceneConfig {
    static constexpr GLint WIDTH = 1200;
    static constexpr GLint HEIGHT = 800;
    static constexpr float SPOT_OUTER_DEG = 40.0f;
    static constexpr float ROOM_SPACING = 20.0f;
    static constexpr unsigned int SHADOW_SIZE = 1024;
    static constexpr float SHADOW_FAR = 20.0f;
    static constexpr unsigned int SPOT_SHADOW_RES = 1024;
    static constexpr int SPOT_COUNT = 5;
    static constexpr int SHADOW_UPDATE_INTERVAL = 6;
};

// ========== FUNCIÓN PRINCIPAL ORIGINAL (COMPLETA) ==========
int main()
{
    // Window dimensions
    constexpr GLint WIDTH = 1200;
    constexpr GLint HEIGHT = 800;
    const float spotOuterDeg = 40.0f;

    auto main_window = Window::create(WIDTH, HEIGHT, "The Room");
    if (main_window == nullptr) return EXIT_FAILURE;

    create_shaders_program();

    // --- FRUSTUM CULLING VARIABLES ---
    Frustum frustum;
    bool cullingEnabled = true;

    // Place camera inside the room
    Camera camera{glm::vec3{0.f, 1.f, 0.f}, glm::vec3{0.f, 1.f, 0.f}, -90.f, 0.f, 5.f, 0.15f};

    // Create the rooms (center + four directions)
    std::vector<Room> rooms;
    rooms.emplace_back(Data::root_path, Room::DOOR_FRONT | Room::DOOR_BACK | Room::DOOR_LEFT | Room::DOOR_RIGHT);
    rooms.emplace_back(Data::root_path, Room::DOOR_LEFT | Room::DOOR_FRONT | Room::DOOR_BACK);
    rooms.emplace_back(Data::root_path, Room::DOOR_RIGHT);
    rooms.emplace_back(Data::root_path, Room::DOOR_BACK);
    rooms.emplace_back(Data::root_path, Room::DOOR_FRONT);

    // Prepare transforms for multiple room instances
    std::vector<glm::mat4> roomTransforms;
    const float roomSpacing = 20.0f;
    roomTransforms.push_back(glm::mat4(1.0f));
    roomTransforms.push_back(glm::translate(glm::mat4(1.0f), glm::vec3(roomSpacing, 0.0f, 0.0f)));
    roomTransforms.push_back(glm::translate(glm::mat4(1.0f), glm::vec3(-roomSpacing, 0.0f, 0.0f)));
    roomTransforms.push_back(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, roomSpacing)));
    roomTransforms.push_back(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -roomSpacing)));

    // Create the shared mesh for all lightbulbs
    Lightbulb::create_mesh();

    // Create fallback textures
    auto fallback_albedo = std::make_shared<Texture>(255, 255, 255, 255);
    fallback_albedo->load();
    auto fallback_normal = std::make_shared<Texture>(128, 128, 255, 255);
    fallback_normal->load();

    // ========== CARGAR TODOS LOS MODELOS ORIGINALES ==========
    std::cout << "Loading: wooden_table_02_1k.gltf" << std::endl;
    std::vector<AssimpLoader::Renderable> imported_models = AssimpLoader::loadModel(Data::root_path / "models" / "wooden_table_02_1k.gltf");
    UIResponsiveWhileLoading(main_window);

    std::vector<std::vector<AssimpLoader::Renderable>> prop_models;
    std::vector<std::string> prop_files = {
        "lubricant_spray_1k.gltf", "marble_bust_01_1k.gltf", "rubber_duck_toy_1k.gltf", "street_rat_1k.gltf"
    };
    for (auto &pf : prop_files) {
        auto group = AssimpLoader::loadModel(Data::root_path / "models" / pf);
        prop_models.push_back(std::move(group));
        UIResponsiveWhileLoading(main_window);
    }

    std::cout << "Loading: potted_plant_01_1k.gltf" << std::endl;
    std::vector<AssimpLoader::Renderable> potted_models = AssimpLoader::loadModel(Data::root_path / "models" / "potted_plant_01_1k.gltf");
    UIResponsiveWhileLoading(main_window);

    std::cout << "Loading: fancy_picture_frame_01_1k.gltf" << std::endl;
    std::vector<AssimpLoader::Renderable> picture_models = AssimpLoader::loadModel(Data::root_path / "models" / "fancy_picture_frame_01_1k.gltf");
    UIResponsiveWhileLoading(main_window);

    std::cout << "Loading: fancy_picture_frame_02_1k.gltf" << std::endl;
    std::vector<AssimpLoader::Renderable> picture2_models = AssimpLoader::loadModel(Data::root_path / "models" / "fancy_picture_frame_02_1k.gltf");
    UIResponsiveWhileLoading(main_window);

    std::cout << "Loading: concrete_cat_statue_1k.gltf" << std::endl;
    std::vector<AssimpLoader::Renderable> cat_statue = AssimpLoader::loadModel(Data::root_path / "models" / "concrete_cat_statue_1k.gltf");
    UIResponsiveWhileLoading(main_window);

    std::cout << "Loading: cannon_01_1k.gltf" << std::endl;
    std::vector<AssimpLoader::Renderable> cannon_statue = AssimpLoader::loadModel(Data::root_path / "models" / "cannon_01_1k.gltf");
    UIResponsiveWhileLoading(main_window);

    std::cout << "Loading: CoffeeCart_01_1k.gltf" << std::endl;
    std::vector<AssimpLoader::Renderable> cart_statue = AssimpLoader::loadModel(Data::root_path / "models" / "CoffeeCart_01_1k.gltf");
    UIResponsiveWhileLoading(main_window);

    std::cout << "Loading: Drill_01_1k.gltf" << std::endl;
    std::vector<AssimpLoader::Renderable> drill_statue = AssimpLoader::loadModel(Data::root_path / "models" / "Drill_01_1k.gltf");
    UIResponsiveWhileLoading(main_window);

    std::cout << "Loading: horse_head_1k.gltf" << std::endl;
    std::vector<AssimpLoader::Renderable> horse_statue = AssimpLoader::loadModel(Data::root_path / "models" / "horse_head_1k.gltf");
    UIResponsiveWhileLoading(main_window);

    std::cout << "Loading: potted_plant_02_1k.gltf" << std::endl;
    std::vector<AssimpLoader::Renderable> potted_plant_02 = AssimpLoader::loadModel(Data::root_path / "models" / "potted_plant_02_1k.gltf");
    UIResponsiveWhileLoading(main_window);

    // ========== CONFIGURACIÓN DE LUCES ORIGINAL ==========
    std::vector<Lightbulb> lightbulbs;
    PointLight ceilingLight(glm::vec3{0.0f, 7.5f, 0.0f},
                           glm::vec3{0.05f, 0.05f, 0.05f}, glm::vec3{2.0f, 2.0f, 2.0f}, glm::vec3{0.5f, 0.5f, 0.5f},
                           1.0f, 0.09f, 0.032f);
    lightbulbs.emplace_back(ceilingLight, glm::vec3{1.0f, 1.0f, 1.0f});

    // ========== SHADOW MAPPING ORIGINAL ==========
    unsigned int SHADOW_SIZE = 1024;
    const float SHADOW_FAR = 20.0f;

    for (int i = 0; i < 8; ++i) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    }

    auto depthShader = Shader::create_from_files(Data::root_path / "shaders" / "depth_cube.vert", Data::root_path / "shaders" / "depth_cube.frag");
    ShadowCubemap shadowCubemap(SHADOW_SIZE, SHADOW_FAR);

    const int SPOT_COUNT = 5;
    const unsigned int SPOT_SHADOW_RES = 1024;
    std::vector<GLuint> spotDepthMaps(SPOT_COUNT, 0);
    std::vector<GLuint> spotDepthFBOs(SPOT_COUNT, 0);
    
    for (int i = 0; i < SPOT_COUNT; ++i) {
        glGenTextures(1, &spotDepthMaps[i]);
        glBindTexture(GL_TEXTURE_2D, spotDepthMaps[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SPOT_SHADOW_RES, SPOT_SHADOW_RES, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
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
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "Spot shadow FBO not complete!" << std::endl;
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        spotDepthFBOs[i] = fbo;
    }

    auto spotDepthShader = Shader::create_from_files(Data::root_path / "shaders" / "spot_depth.vert", Data::root_path / "shaders" / "spot_depth.frag");
    const bool enableShadows = true;

    Data::sky_box = std::make_shared<SkyBox>(
        Data::root_path,
        std::vector<fs::path>{"px.png", "nx.png", "py.png", "ny.png", "pz.png", "nz.png"}
    );

    glm::mat4 projection = glm::perspective(45.f, main_window->get_aspect_ratio(), 0.1f, 100.f);
    GLfloat last_time = glfwGetTime();
    static int shadowUpdateCounter = 0;
    const int SHADOW_UPDATE_INTERVAL = 6;

    initialize_exterior_floor();

    // ========== BUCLE PRINCIPAL ORIGINAL ==========
    while (!main_window->should_be_closed()) {
        GLfloat now = glfwGetTime();
        GLfloat dt = now - last_time;
        last_time = now;

        glfwPollEvents();

        // Actualizar cámara y frustum
        glm::mat4 view = camera.get_view_matrix();
        glm::mat4 viewProj = projection * view;
        frustum.update(viewProj);

        glEnable(GL_DEPTH_TEST);
        camera.handle_keys(main_window->get_keys());
        camera.handle_mouse(main_window->get_x_change(), main_window->get_y_change());
        camera.update(dt);

        // Mover luz con teclado (ORIGINAL)
        const auto &keys = main_window->get_keys();
        glm::vec3 lp = ceilingLight.get_position();
        const float lightSpeed = 3.0f;
        if (keys[GLFW_KEY_UP]) lp.z -= lightSpeed * dt;
        if (keys[GLFW_KEY_DOWN]) lp.z += lightSpeed * dt;
        if (keys[GLFW_KEY_LEFT]) lp.x -= lightSpeed * dt;
        if (keys[GLFW_KEY_RIGHT]) lp.x += lightSpeed * dt;
        if (keys[GLFW_KEY_PERIOD]) lp.y += lightSpeed * dt;
        if (keys[GLFW_KEY_COMMA]) lp.y -= lightSpeed * dt;
        
        static glm::vec3 prevLp = lp;
        if (lp != prevLp) {
            ceilingLight.set_position(lp);
            if (!lightbulbs.empty()) lightbulbs[0].set_position(lp);
            prevLp = lp;
        }

        // Shadow passes (ORIGINAL)
        bool updateShadowsThisFrame = (shadowUpdateCounter % SHADOW_UPDATE_INTERVAL == 0);
        shadowUpdateCounter++;

        // --- Shadow pass for the single point light ---
        if (enableShadows && updateShadowsThisFrame) {
            glm::vec3 light_pos = ceilingLight.get_position();
            depthShader->use();
            float near_plane = 0.1f;
            glm::mat4 shadow_proj = glm::perspective(glm::radians(90.0f), 1.0f, near_plane, SHADOW_FAR);
            auto shadow_views = shadowCubemap.get_shadow_views(light_pos);

            glViewport(0, 0, SHADOW_SIZE, SHADOW_SIZE);
            glBindFramebuffer(GL_FRAMEBUFFER, shadowCubemap.get_fbo());
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
            
            for (unsigned int face = 0; face < 6; ++face) {
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, shadowCubemap.get_depth_cubemap_id(), 0);
                glClear(GL_DEPTH_BUFFER_BIT);

                glUniformMatrix4fv(depthShader->get_uniform_projection_id(), 1, GL_FALSE, glm::value_ptr(shadow_proj));
                glUniformMatrix4fv(depthShader->get_uniform_view_id(), 1, GL_FALSE, glm::value_ptr(shadow_views[face]));
                glUniform3fv(glGetUniformLocation(depthShader->get_program_id(), "lightPos"), 1, glm::value_ptr(light_pos));
                glUniform1f(glGetUniformLocation(depthShader->get_program_id(), "far_plane"), SHADOW_FAR);

                // Render rooms for depth
                for (size_t ri = 0; ri < rooms.size() && ri < roomTransforms.size(); ++ri) {
                    glm::vec3 roomCenter = glm::vec3(roomTransforms[ri] * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
                    float roomRadius = 15.0f;
                    if (frustum.isSphereInFrustum(roomCenter, roomRadius)) {
                        rooms[ri].render_for_depth(depthShader, roomTransforms[ri]);
                    }
                }

                // Render imported models for depth (CÓDIGO ORIGINAL COMPLETO)
                if (!imported_models.empty()) {
                    const float modelScale = 2.0f;
                    const float floorY = -2.0f;
                    std::vector<glm::vec3> positions = {
                        glm::vec3(0.0f, floorY, 28.5f), glm::vec3(0.0f, floorY, -28.5f),
                        glm::vec3(28.5f, floorY, 0.0f), glm::vec3(-28.5f, floorY, 0.0f)
                    };
                    std::vector<float> rotations = {0.0f, glm::pi<float>(), glm::radians(-90.0f), glm::radians(90.0f)};

                    for (auto &r : imported_models) {
                        for (size_t i = 0; i < positions.size(); ++i) {
                            glm::vec3 tablePos = positions[i];
                            float tableRadius = 5.0f;
                            float distToLight = glm::length(tablePos - light_pos);
                            if (distToLight > SHADOW_FAR + 5.0f) continue;

                            if (!cullingEnabled || frustum.isSphereInFrustum(tablePos, tableRadius)) {
                                glm::mat4 modelMat{1.0f};
                                modelMat = glm::translate(modelMat, positions[i]);
                                modelMat = glm::rotate(modelMat, rotations[i], glm::vec3(0.0f, 1.0f, 0.0f));
                                modelMat = glm::scale(modelMat, glm::vec3(modelScale));
                                modelMat = modelMat * r.transform;
                                glUniformMatrix4fv(depthShader->get_uniform_model_id(), 1, GL_FALSE, glm::value_ptr(modelMat));
                                r.mesh->render();
                            }
                        }
                    }

                    // Render potted plants for depth
                    if (!potted_models.empty()) {
                        std::vector<glm::vec3> potPositions = {
                            glm::vec3(8.0f, floorY, 8.0f), glm::vec3(-8.0f, floorY, 8.0f),
                            glm::vec3(8.0f, floorY, -8.0f), glm::vec3(-8.0f, floorY, -8.0f)
                        };
                        std::vector<float> potRot = {0.0f, glm::pi<float>(), glm::radians(90.0f), glm::radians(-90.0f)};
                        const float potScale = 4.0f;

                        for (auto &pr : potted_models) {
                            for (size_t i = 0; i < potPositions.size(); ++i) {
                                if (!cullingEnabled || frustum.isSphereInFrustum(potPositions[i], 2.0f)) {
                                    float dist = glm::length(potPositions[i] - light_pos);
                                    if (dist > SHADOW_FAR + 5.0f) continue;
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
                    }

                    // Render picture frames for depth
                    if (!picture_models.empty()) {
                        const float pictureYOffset = 3.5f;
                        const float pictureShift = 4.0f;
                        const float wallDist = 9.8f;
                        std::vector<glm::vec3> picPositions = {
                            glm::vec3(pictureShift, floorY + pictureYOffset, wallDist),
                            glm::vec3(pictureShift, floorY + pictureYOffset, -wallDist),
                            glm::vec3(wallDist, floorY + pictureYOffset, pictureShift),
                            glm::vec3(-wallDist, floorY + pictureYOffset, pictureShift)
                        };
                        std::vector<float> picRot = {glm::pi<float>(), 0.0f, glm::radians(270.0f), glm::radians(90.0f)};
                        const float picScale = 4.0f;

                        renderModelSpotShadow(picture_models, picPositions, picRot, cullingEnabled, frustum, 2.0f, picScale, depthShader);

                        if (!picture2_models.empty()) {
                            std::vector<glm::vec3> pic2Positions = {
                                glm::vec3(-pictureShift, floorY + pictureYOffset, wallDist),
                                glm::vec3(-pictureShift, floorY + pictureYOffset, -wallDist),
                                glm::vec3(wallDist, floorY + pictureYOffset, -pictureShift),
                                glm::vec3(-wallDist, floorY + pictureYOffset, -pictureShift)
                            };
                            renderModelSpotShadow(picture2_models, pic2Positions, picRot, cullingEnabled, frustum, 2.0f, picScale, depthShader);
                        }
                    }

                    // Render props for depth
                    float tableHeight = 0.0f;
                    for (auto &r : imported_models) {
                        tableHeight = std::max(tableHeight, r.src_max.y - r.src_min.y);
                    }
                    tableHeight *= modelScale;

                    std::vector<float> perPropFootprint = {0.15f, 0.6f, 0.8f, 1.5f};
                    for (size_t i = 0; i < prop_models.size() && i < positions.size(); ++i) {
                        auto &group = prop_models[i];
                        glm::vec3 basePos = positions[i];
                        basePos.y = floorY + tableHeight + 0.02f;
                        for (auto &pr : group) {
                            if (!cullingEnabled || frustum.isSphereInFrustum(basePos, 1.0f)) {
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

                                glUniformMatrix4fv(depthShader->get_uniform_model_id(), 1, GL_FALSE, glm::value_ptr(modelMat));
                                pr.mesh->render();
                            }
                        }
                    }
                }

                // Render big statues for depth (ORIGINAL)
                const float defaultStatueScale = 12.0f;
                const float cannonScale = 3.0f;
                const float coffeeScale = 2.0f;
                const float floorY = -2.0f;

                if (!cat_statue.empty()) {
                    renderModelPointShadow(cat_statue, glm::vec3(0.0f, floorY, 0.0f), cullingEnabled, frustum, 8.0f, defaultStatueScale, depthShader);
                }
                if (!cannon_statue.empty()) {
                    renderModelPointShadow(cannon_statue, glm::vec3(20.0f, floorY, 0.0f), cullingEnabled, frustum, 5.0f, cannonScale, depthShader);
                }
                if (!cart_statue.empty()) {
                    renderModelPointShadow(cart_statue, glm::vec3(-20.0f, floorY, 0.0f), cullingEnabled, frustum, 4.0f, coffeeScale, depthShader);
                }
                if (!drill_statue.empty()) {
                    renderModelPointShadow(drill_statue, glm::vec3(0.0f, floorY, 20.0f), cullingEnabled, frustum, 8.0f, defaultStatueScale, depthShader);
                }
                if (!horse_statue.empty()) {
                    renderModelPointShadow(horse_statue, glm::vec3(0.0f, floorY, -20.0f), cullingEnabled, frustum, 8.0f, defaultStatueScale, depthShader);
                }

                // Render potted plants in outer rooms for depth
                if (!potted_plant_02.empty()) {
                    const float potScale = 4.0f;
                    std::vector<glm::vec3> outerRoomCenters = {
                        glm::vec3(20.0f, 0.0f, 0.0f), glm::vec3(-20.0f, 0.0f, 0.0f),
                        glm::vec3(0.0f, 0.0f, 20.0f), glm::vec3(0.0f, 0.0f, -20.0f)
                    };
                    std::vector<glm::vec3> cornerOffsets = {
                        glm::vec3(8.0f, 0.0f, 8.0f), glm::vec3(-8.0f, 0.0f, 8.0f),
                        glm::vec3(8.0f, 0.0f, -8.0f), glm::vec3(-8.0f, 0.0f, -8.0f)
                    };

                    for (const auto &center : outerRoomCenters) {
                        for (const auto &offset : cornerOffsets) {
                            glm::vec3 pos = center + offset;
                            pos.y = floorY;
                            if (!cullingEnabled || frustum.isSphereInFrustum(pos, 2.0f)) {
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
            }
            glDisable(GL_CULL_FACE);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(0, 0, main_window->get_buffer_width(), main_window->get_buffer_height());
        }

        // --- Spot shadow pass ---
        if (enableShadows && updateShadowsThisFrame) {
            const float spotFov = spotOuterDeg * 2.0f;
            const float near_plane_spot = 0.1f;
            const float far_plane_spot = 25.0f;

            for (int si = 0; si < (int)roomTransforms.size() && si < (int)spotDepthFBOs.size(); ++si) {
                glm::vec3 spos = glm::vec3(roomTransforms[si] * glm::vec4(0.0f, 7.5f, 0.0f, 1.0f));
                glm::vec3 sdir = glm::vec3(0.0f, -1.0f, 0.0f);

                glm::mat4 lightProj = glm::perspective(glm::radians(spotFov), 1.0f, near_plane_spot, far_plane_spot);
                glm::vec3 up = fabs(sdir.y) > 0.99f ? glm::vec3(0.0f, 0.0f, 1.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
                glm::mat4 lightView = glm::lookAt(spos, spos + sdir, up);
                glm::mat4 lightSpace = lightProj * lightView;

                glViewport(0, 0, SPOT_SHADOW_RES, SPOT_SHADOW_RES);
                glBindFramebuffer(GL_FRAMEBUFFER, spotDepthFBOs[si]);
                glClear(GL_DEPTH_BUFFER_BIT);
                glDisable(GL_CULL_FACE);

                spotDepthShader->use();
                glUniformMatrix4fv(glGetUniformLocation(spotDepthShader->get_program_id(), "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpace));

                // Render rooms for spot depth
                for (size_t ri = 0; ri < rooms.size() && ri < roomTransforms.size(); ++ri) {
                    glm::vec3 roomCenter = glm::vec3(roomTransforms[ri] * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
                    float roomRadius = 15.0f;
                    if (frustum.isSphereInFrustum(roomCenter, roomRadius)) {
                        rooms[ri].render_for_depth(spotDepthShader, roomTransforms[ri]);
                    }
                }

                // Render imported models for spot depth (CÓDIGO ORIGINAL)
                if (!imported_models.empty()) {
                    const float modelScale = 2.0f;
                    const float floorY = -2.0f;
                    std::vector<glm::vec3> positions = {
                        glm::vec3(0.0f, floorY, 28.5f), glm::vec3(0.0f, floorY, -28.5f),
                        glm::vec3(28.5f, floorY, 0.0f), glm::vec3(-28.5f, floorY, 0.0f)
                    };
                    std::vector<float> rotations = {0.0f, glm::pi<float>(), glm::radians(-90.0f), glm::radians(90.0f)};

                    for (auto &r : imported_models) {
                        for (size_t i = 0; i < positions.size(); ++i) {
                            glm::vec3 tablePos = positions[i];
                            float tableRadius = 5.0f;
                            float distToSpot = glm::length(tablePos - spos);
                            if (distToSpot > far_plane_spot + 5.0f) continue;
                            
                            glm::vec3 toObj = glm::normalize(tablePos - spos);
                            glm::vec3 spotDir = glm::normalize(sdir);
                            float angleDeg = glm::degrees(acos(glm::clamp(glm::dot(spotDir, toObj), -1.0f, 1.0f)));
                            if (angleDeg > (spotOuterDeg * 1.2f)) continue;

                            if (!cullingEnabled || frustum.isSphereInFrustum(tablePos, tableRadius)) {
                                glm::mat4 modelMat{1.0f};
                                modelMat = glm::translate(modelMat, positions[i]);
                                modelMat = glm::rotate(modelMat, rotations[i], glm::vec3(0.0f, 1.0f, 0.0f));
                                modelMat = glm::scale(modelMat, glm::vec3(modelScale));
                                modelMat = modelMat * r.transform;
                                glUniformMatrix4fv(spotDepthShader->get_uniform_model_id(), 1, GL_FALSE, glm::value_ptr(modelMat));
                                r.mesh->render();
                            }
                        }
                    }

                    // ... (continuar con el resto del código original de spot shadows)
                }

                glDisable(GL_CULL_FACE);
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                glViewport(0, 0, main_window->get_buffer_width(), main_window->get_buffer_height());

                // Upload lightSpace matrix
                Data::shader_list[0]->use();
                std::string name = "spotLightSpaceMatrices[" + std::to_string(si) + "]";
                glUniformMatrix4fv(glGetUniformLocation(Data::shader_list[0]->get_program_id(), name.c_str()), 1, GL_FALSE, glm::value_ptr(lightSpace));
            }
        }

        // ========== RENDER PRINCIPAL ORIGINAL ==========
        glClearColor(0.f, 0.f, 0.f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 1. Skybox
        glDepthFunc(GL_LEQUAL);
        Data::sky_box->render(camera.get_view_matrix(), projection);
        glDepthFunc(GL_LESS);
        
        // 2. Piso exterior
        render_exterior_floor(camera.get_view_matrix(), projection, camera.get_position());
        
        // 3. Habitaciones y objetos
        Data::shader_list[0]->use();

        // REACTIVAR CONFIGURACIONES ORIGINALES
        glUniform1i(Data::shader_list[0]->get_uniform_texture_sampler_id(), 0);
        glUniform1i(glGetUniformLocation(Data::shader_list[0]->get_program_id(), "normal_sampler"), 1);
        glUniform1i(glGetUniformLocation(Data::shader_list[0]->get_program_id(), "enableShadows"), enableShadows ? 1 : 0);

        // RESTAURAR LUCES ORIGINALES
        glUniform3f(glGetUniformLocation(Data::shader_list[0]->get_program_id(), "dirLight.direction"), -0.2f, -1.0f, -0.3f);
        glUniform3f(glGetUniformLocation(Data::shader_list[0]->get_program_id(), "dirLight.diffuse"), 0.5f, 0.5f, 0.5f);
        glUniform3f(glGetUniformLocation(Data::shader_list[0]->get_program_id(), "dirLight.specular"), 0.1f, 0.1f, 0.1f);

        // Shadow maps
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_CUBE_MAP, shadowCubemap.get_depth_cubemap_id());
        glUniform1i(glGetUniformLocation(Data::shader_list[0]->get_program_id(), "shadowMap"), 3);
        glUniform1f(glGetUniformLocation(Data::shader_list[0]->get_program_id(), "far_plane"), SHADOW_FAR);
        glUniform1f(glGetUniformLocation(Data::shader_list[0]->get_program_id(), "shadowRadius"), 0.12f);

        // Matrices
        glUniformMatrix4fv(Data::shader_list[0]->get_uniform_projection_id(), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(Data::shader_list[0]->get_uniform_view_id(), 1, GL_FALSE, glm::value_ptr(camera.get_view_matrix()));
        glUniform3fv(glGetUniformLocation(Data::shader_list[0]->get_program_id(), "viewPosition"), 1, glm::value_ptr(camera.get_position()));

        // Point lights
        for (size_t i = 0; i < lightbulbs.size(); ++i) {
            lightbulbs[i].use_light(Data::shader_list[0], i);
        }

        // Spotlights ORIGINALES
        for (size_t si = 0; si < roomTransforms.size() && si < 5; ++si) {
            glm::vec3 spos = glm::vec3(roomTransforms[si] * glm::vec4(0.0f, 7.5f, 0.0f, 1.0f));
            glm::vec3 sdir = glm::vec3(0.0f, -1.0f, 0.0f);

            std::string base = "spotLights[" + std::to_string(si) + "]";
            glUniform3fv(glGetUniformLocation(Data::shader_list[0]->get_program_id(), (base + ".position").c_str()), 1, glm::value_ptr(spos));
            glUniform3fv(glGetUniformLocation(Data::shader_list[0]->get_program_id(), (base + ".direction").c_str()), 1, glm::value_ptr(sdir));
            glUniform1f(glGetUniformLocation(Data::shader_list[0]->get_program_id(), (base + ".cutOff").c_str()), cos(glm::radians(30.0f)));
            glUniform1f(glGetUniformLocation(Data::shader_list[0]->get_program_id(), (base + ".outerCutOff").c_str()), cos(glm::radians(spotOuterDeg)));
            glUniform1f(glGetUniformLocation(Data::shader_list[0]->get_program_id(), (base + ".constant").c_str()), 1.0f);
            glUniform1f(glGetUniformLocation(Data::shader_list[0]->get_program_id(), (base + ".linear").c_str()), 0.09f);
            glUniform1f(glGetUniformLocation(Data::shader_list[0]->get_program_id(), (base + ".quadratic").c_str()), 0.032f);
            glUniform3f(glGetUniformLocation(Data::shader_list[0]->get_program_id(), (base + ".ambient").c_str()), 0.02f, 0.02f, 0.02f);
            glUniform3f(glGetUniformLocation(Data::shader_list[0]->get_program_id(), (base + ".diffuse").c_str()), 3.0f, 3.0f, 2.7f);
            glUniform3f(glGetUniformLocation(Data::shader_list[0]->get_program_id(), (base + ".specular").c_str()), 1.0f, 1.0f, 1.0f);
        }

        // Spot shadow maps
        for (int si = 0; si < (int)spotDepthMaps.size(); ++si) {
            glActiveTexture(GL_TEXTURE4 + si);
            glBindTexture(GL_TEXTURE_2D, spotDepthMaps[si]);
            std::string name = "spotShadowMaps[" + std::to_string(si) + "]";
            glUniform1i(glGetUniformLocation(Data::shader_list[0]->get_program_id(), name.c_str()), 4 + si);
        }

        // Render rooms
        for (size_t i = 0; i < roomTransforms.size() && i < rooms.size(); ++i) {
            glm::vec3 roomCenter = glm::vec3(roomTransforms[i] * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
            float roomRadius = 15.0f;
            if (!cullingEnabled || frustum.isSphereInFrustum(roomCenter, roomRadius)) {
                rooms[i].render(Data::shader_list[0], roomTransforms[i]);
            }
        }

        // Render imported models (CÓDIGO ORIGINAL COMPLETO)
        if (!imported_models.empty()) {
            Data::shader_list[0]->use();
            glUniform1i(Data::shader_list[0]->get_uniform_texture_sampler_id(), 0);
            glUniform1i(glGetUniformLocation(Data::shader_list[0]->get_program_id(), "normal_sampler"), 1);
            glUniform1f(glGetUniformLocation(Data::shader_list[0]->get_program_id(), "material.shininess"), 32.0f);

            const float modelScale = 2.0f;
            const float floorY = -2.0f;
            std::vector<glm::vec3> positions = {
                glm::vec3(0.0f, floorY, 28.5f), glm::vec3(0.0f, floorY, -28.5f),
                glm::vec3(28.5f, floorY, 0.0f), glm::vec3(-28.5f, floorY, 0.0f)
            };
            std::vector<float> rotations = {
                0.0f, glm::pi<float>(), glm::radians(-90.0f), glm::radians(90.0f)
            };

            placeModelInPosition(imported_models, positions, rotations, cullingEnabled, frustum, 5.0f, modelScale, fallback_albedo, fallback_normal);

            // Render props (ORIGINAL)
            float tableHeight = 0.0f;
            for (auto &r : imported_models) {
                tableHeight = std::max(tableHeight, r.src_max.y - r.src_min.y);
            }
            tableHeight *= modelScale;

            std::vector<float> perPropFootprintMain = {0.15f, 0.6f, 0.8f, 1.5f};
            for (size_t i = 0; i < prop_models.size() && i < positions.size(); ++i) {
                auto &group = prop_models[i];
                glm::vec3 basePos = positions[i];
                basePos.y = floorY + tableHeight + 0.02f;
                for (auto &pr : group) {
                    if (!cullingEnabled || frustum.isSphereInFrustum(basePos, 1.0f)) {
                        glm::vec3 srcSize = pr.src_max - pr.src_min;
                        glm::vec3 srcCenter = (pr.src_min + pr.src_max) * 0.5f;
                        float footprintDim = std::max(0.001f, std::max(srcSize.x, srcSize.z));
                        float target = (i < perPropFootprintMain.size()) ? perPropFootprintMain[i] : 0.6f;
                        float scaleUniform = target / footprintDim;
                        scaleUniform = std::clamp(scaleUniform, 0.02f, 10.0f);

                        glm::mat4 modelMat{1.0f};
                        modelMat = glm::translate(modelMat, basePos);
                        modelMat = glm::scale(modelMat, glm::vec3(scaleUniform));
                        glm::vec3 centerXZ = glm::vec3(srcCenter.x, 0.0f, srcCenter.z);
                        modelMat = modelMat * pr.transform * glm::translate(glm::mat4(1.0f), -centerXZ);

                        glUniformMatrix4fv(Data::shader_list[0]->get_uniform_model_id(), 1, GL_FALSE, glm::value_ptr(modelMat));

                        if (pr.albedo) pr.albedo->use();
                        else fallback_albedo->use();
                        glActiveTexture(GL_TEXTURE1);
                        if (pr.normal) glBindTexture(GL_TEXTURE_2D, pr.normal->get_id());
                        else glBindTexture(GL_TEXTURE_2D, fallback_normal->get_id());

                        pr.mesh->render();
                    }
                }
            }

            // Render potted plants (ORIGINAL)
            if (!potted_models.empty()) {
                std::vector<glm::vec3> potPositions = {
                    glm::vec3(8.0f, floorY, 8.0f), glm::vec3(-8.0f, floorY, 8.0f),
                    glm::vec3(8.0f, floorY, -8.0f), glm::vec3(-8.0f, floorY, -8.0f)
                };
                std::vector<float> potRot = {0.0f, glm::pi<float>(), glm::radians(90.0f), glm::radians(-90.0f)};
                const float potScale = 4.0f;

                placeModelInPosition(potted_models, potPositions, potRot, cullingEnabled, frustum, 2.0f, potScale, fallback_albedo, fallback_normal);
            }

            // Render picture frames (ORIGINAL)
            if (!picture_models.empty()) {
                const float pictureYOffset = 3.5f;
                const float pictureShift = 4.0f;
                const float wallDist = 9.8f;
                std::vector<glm::vec3> picPositions = {
                    glm::vec3(pictureShift, floorY + pictureYOffset, wallDist),
                    glm::vec3(pictureShift, floorY + pictureYOffset, -wallDist),
                    glm::vec3(wallDist, floorY + pictureYOffset, pictureShift),
                    glm::vec3(-wallDist, floorY + pictureYOffset, pictureShift)
                };
                std::vector<float> picRot = {glm::pi<float>(), 0.0f, glm::radians(270.0f), glm::radians(90.0f)};
                const float picScale = 4.0f;

                placeModelInPosition(picture_models, picPositions, picRot, cullingEnabled, frustum, 2.0f, picScale, fallback_albedo, fallback_normal);

                if (!picture2_models.empty()) {
                    std::vector<glm::vec3> pic2Positions = {
                        glm::vec3(-pictureShift, floorY + pictureYOffset, wallDist),
                        glm::vec3(-pictureShift, floorY + pictureYOffset, -wallDist),
                        glm::vec3(wallDist, floorY + pictureYOffset, -pictureShift),
                        glm::vec3(-wallDist, floorY + pictureYOffset, -pictureShift)
                    };
                    placeModelInPosition(picture2_models, pic2Positions, picRot, cullingEnabled, frustum, 2.0f, picScale, fallback_albedo, fallback_normal);
                }
            }

            // Render big statues (ORIGINAL)
            const float defaultStatueScale = 12.0f;
            const float cannonScale = 3.0f;
            const float coffeeScale = 2.0f;

            if (!cat_statue.empty()) {
                placeModelInPosition(cat_statue, glm::vec3(0.0f, floorY, 0.0f), cullingEnabled, frustum, 8.0f, defaultStatueScale, fallback_albedo, fallback_normal);
            }
            if (!cannon_statue.empty()) {
                placeModelInPosition(cannon_statue, glm::vec3(20.0f, floorY, 0.0f), cullingEnabled, frustum, 5.0f, cannonScale, fallback_albedo, fallback_normal);
            }
            if (!cart_statue.empty()) {
                placeModelInPosition(cart_statue, glm::vec3(-20.0f, floorY, 0.0f), cullingEnabled, frustum, 4.0f, coffeeScale, fallback_albedo, fallback_normal);
            }
            if (!drill_statue.empty()) {
                placeModelInPosition(drill_statue, glm::vec3(0.0f, floorY, 20.0f), cullingEnabled, frustum, 8.0f, defaultStatueScale, fallback_albedo, fallback_normal);
            }
            if (!horse_statue.empty()) {
                placeModelInPosition(horse_statue, glm::vec3(0.0f, floorY, -20.0f), cullingEnabled, frustum, 8.0f, defaultStatueScale, fallback_albedo, fallback_normal);
            }

            // Render potted plants in outer rooms (ORIGINAL)
            if (!potted_plant_02.empty()) {
                const float potScale = 4.0f;
                std::vector<glm::vec3> outerRoomCenters = {
                    glm::vec3(20.0f, 0.0f, 0.0f), glm::vec3(-20.0f, 0.0f, 0.0f),
                    glm::vec3(0.0f, 0.0f, 20.0f), glm::vec3(0.0f, 0.0f, -20.0f)
                };
                std::vector<glm::vec3> cornerOffsets = {
                    glm::vec3(8.0f, 0.0f, 8.0f), glm::vec3(-8.0f, 0.0f, 8.0f),
                    glm::vec3(8.0f, 0.0f, -8.0f), glm::vec3(-8.0f, 0.0f, -8.0f)
                };

                for (const auto &center : outerRoomCenters) {
                    for (const auto &offset : cornerOffsets) {
                        glm::vec3 pos = center + offset;
                        pos.y = floorY;
                        placeModelInPosition(potted_plant_02, pos, cullingEnabled, frustum, 2.0f, potScale, fallback_albedo, fallback_normal);
                    }
                }
            }
        }

        // Render lightbulbs
        Data::shader_list[1]->use();
        glUniformMatrix4fv(glGetUniformLocation(Data::shader_list[1]->get_program_id(), "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(Data::shader_list[1]->get_program_id(), "view"), 1, GL_FALSE, glm::value_ptr(camera.get_view_matrix()));

        for (const auto &bulb : lightbulbs) {
            bulb.render(Data::shader_list[1]);
        }

        glUseProgram(0);
        main_window->swap_buffers();
    }

    return EXIT_SUCCESS;
}