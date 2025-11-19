#include <AssimpLoader.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <Mesh.hpp>
#include <Texture.hpp>
#include <BSlogger.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <filesystem>
#include <limits>

namespace AssimpLoader
{
    // Helper to convert Assimp matrix to GLM
    glm::mat4 aiMatrix4x4ToGlm(const aiMatrix4x4& from)
    {
        glm::mat4 to;
        // Assimp is row-major, GLM is column-major.
        to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = from.a4;
        to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = from.b4;
        to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = from.c4;
        to[0][3] = from.d1; to[1][3] = from.d2; to[2][3] = from.d3; to[3][3] = from.d4;
        return to;
    }

    void processNode(aiNode* node, const aiScene* scene, glm::mat4 parentTransform, 
                     std::vector<AssimpLoader::Renderable>& out, const std::filesystem::path& model_dir)
    {
        glm::mat4 nodeTransform = aiMatrix4x4ToGlm(node->mTransformation);
        glm::mat4 globalTransform = parentTransform * nodeTransform;

        for (unsigned int i = 0; i < node->mNumMeshes; ++i)
        {
            aiMesh* aMesh = scene->mMeshes[node->mMeshes[i]];
            
            // Process Mesh
            std::vector<float> vertices;
            vertices.reserve(aMesh->mNumVertices * 8);

            glm::vec3 minV{ std::numeric_limits<float>::infinity() };
            glm::vec3 maxV{ -std::numeric_limits<float>::infinity() };

            for (unsigned int v = 0; v < aMesh->mNumVertices; ++v)
            {
                aiVector3D pos = aMesh->mVertices[v];
                // Update bounds
                if (pos.x < minV.x) minV.x = pos.x;
                if (pos.y < minV.y) minV.y = pos.y;
                if (pos.z < minV.z) minV.z = pos.z;
                if (pos.x > maxV.x) maxV.x = pos.x;
                if (pos.y > maxV.y) maxV.y = pos.y;
                if (pos.z > maxV.z) maxV.z = pos.z;

                vertices.push_back(pos.x);
                vertices.push_back(pos.y);
                vertices.push_back(pos.z);

                if (aMesh->HasNormals()) {
                    aiVector3D n = aMesh->mNormals[v];
                    vertices.push_back(n.x);
                    vertices.push_back(n.y);
                    vertices.push_back(n.z);
                } else {
                    vertices.push_back(0.0f); vertices.push_back(0.0f); vertices.push_back(0.0f);
                }

                if (aMesh->HasTextureCoords(0)) {
                    aiVector3D uv = aMesh->mTextureCoords[0][v];
                    vertices.push_back(uv.x);
                    vertices.push_back(1.0f - uv.y);
                } else {
                    vertices.push_back(0.0f); vertices.push_back(0.0f);
                }
            }

            std::vector<unsigned int> indices;
            indices.reserve(aMesh->mNumFaces * 3);
            for (unsigned int f = 0; f < aMesh->mNumFaces; ++f) {
                const aiFace& face = aMesh->mFaces[f];
                if (face.mNumIndices == 3) {
                    indices.push_back(face.mIndices[0]);
                    indices.push_back(face.mIndices[1]);
                    indices.push_back(face.mIndices[2]);
                }
            }

            auto mesh = Mesh::create(vertices, indices);

            // Textures
            std::shared_ptr<Texture> albedo_tex = nullptr;
            std::shared_ptr<Texture> normal_tex = nullptr;

            if (scene->mMaterials) {
                aiMaterial* material = scene->mMaterials[aMesh->mMaterialIndex];
                aiString texPath;
                if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS) {
                     std::string tex_rel = texPath.C_Str();
                     if (!tex_rel.empty() && tex_rel[0] != '*') {
                         std::filesystem::path full = model_dir / tex_rel;
                         if (std::filesystem::exists(full)) {
                             albedo_tex = std::make_shared<Texture>(full);
                             albedo_tex->load();
                         } else {
                             LOG_INIT_COUT();
                             log(LOG_WARN) << "AssimpLoader: albedo not found: " << full << "\n";
                         }
                     }
                }
                if (material->GetTexture(aiTextureType_NORMALS, 0, &texPath) == AI_SUCCESS ||
                    material->GetTexture(aiTextureType_HEIGHT, 0, &texPath) == AI_SUCCESS) {
                     std::string tex_rel = texPath.C_Str();
                     if (!tex_rel.empty() && tex_rel[0] != '*') {
                         std::filesystem::path full = model_dir / tex_rel;
                         if (std::filesystem::exists(full)) {
                             normal_tex = std::make_shared<Texture>(full);
                             normal_tex->load();
                         } else {
                             LOG_INIT_COUT();
                             log(LOG_WARN) << "AssimpLoader: normal not found: " << full << "\n";
                         }
                     }
                }
            }

            if (!albedo_tex) {
                albedo_tex = std::make_shared<Texture>(255,255,255,255);
                albedo_tex->load();
            }
            if (!normal_tex) {
                normal_tex = std::make_shared<Texture>(128,128,255,255);
                normal_tex->load();
            }

            AssimpLoader::Renderable r;
            r.mesh = mesh;
            r.albedo = albedo_tex;
            r.normal = normal_tex;
            r.transform = globalTransform; // Use the accumulated transform
            r.src_min = minV;
            r.src_max = maxV;
            
            out.push_back(r);
        }

        for (unsigned int i = 0; i < node->mNumChildren; ++i)
        {
            processNode(node->mChildren[i], scene, globalTransform, out, model_dir);
        }
    }

    std::vector<AssimpLoader::Renderable> loadModel(const std::filesystem::path& path) noexcept
    {
        std::vector<AssimpLoader::Renderable> out;
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path.string(),
            aiProcess_Triangulate |
            aiProcess_GenSmoothNormals |
            aiProcess_JoinIdenticalVertices);

        if (!scene || !scene->mRootNode) return out;

        std::filesystem::path model_dir = path.parent_path();
        
        processNode(scene->mRootNode, scene, glm::mat4(1.0f), out, model_dir);

        // Post-process: Ground the entire model
        if (!out.empty()) {
            float globalMinY = std::numeric_limits<float>::infinity();
            
            for (const auto& r : out) {
                // Transform the 8 corners of the AABB
                glm::vec3 corners[8] = {
                    {r.src_min.x, r.src_min.y, r.src_min.z},
                    {r.src_min.x, r.src_min.y, r.src_max.z},
                    {r.src_min.x, r.src_max.y, r.src_min.z},
                    {r.src_min.x, r.src_max.y, r.src_max.z},
                    {r.src_max.x, r.src_min.y, r.src_min.z},
                    {r.src_max.x, r.src_min.y, r.src_max.z},
                    {r.src_max.x, r.src_max.y, r.src_min.z},
                    {r.src_max.x, r.src_max.y, r.src_max.z}
                };

                for (int i = 0; i < 8; ++i) {
                    glm::vec4 worldPos = r.transform * glm::vec4(corners[i], 1.0f);
                    if (worldPos.y < globalMinY) globalMinY = worldPos.y;
                }
            }

            // Apply offset
            glm::vec3 offset(0.0f, -globalMinY, 0.0f);
            glm::mat4 offsetMat = glm::translate(glm::mat4(1.0f), offset);
            
            for (auto& r : out) {
                r.transform = offsetMat * r.transform;
            }
        }

        return out;
    }
}
