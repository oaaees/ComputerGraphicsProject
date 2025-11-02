#include <AssimpLoader.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <Mesh.hpp>
#include <Texture.hpp>
#include <BSlogger.hpp>

#include <glm/glm.hpp>

#include <filesystem>

namespace AssimpLoader
{
    std::vector<AssimpLoader::Renderable> loadModel(const std::filesystem::path& path) noexcept
    {
        std::vector<AssimpLoader::Renderable> out;

        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path.string(),
            aiProcess_Triangulate |
            aiProcess_GenSmoothNormals |
            aiProcess_JoinIdenticalVertices);

        if (!scene || !scene->HasMeshes())
        {
            return out; // empty
        }

        std::filesystem::path model_dir = path.parent_path();

        for (unsigned int m = 0; m < scene->mNumMeshes; ++m)
        {
            aiMesh* aMesh = scene->mMeshes[m];

            std::vector<float> vertices;
            vertices.reserve(aMesh->mNumVertices * 8);

            // Positions, normals, texcoords (uv)
            for (unsigned int i = 0; i < aMesh->mNumVertices; ++i)
            {
                // position
                aiVector3D pos = aMesh->mVertices[i];
                vertices.push_back(pos.x);
                vertices.push_back(pos.y);
                vertices.push_back(pos.z);

                // normal
                if (aMesh->HasNormals()) {
                    aiVector3D n = aMesh->mNormals[i];
                    vertices.push_back(n.x);
                    vertices.push_back(n.y);
                    vertices.push_back(n.z);
                } else {
                    vertices.push_back(0.0f);
                    vertices.push_back(0.0f);
                    vertices.push_back(0.0f);
                }

                // texcoords (only channel 0)
                if (aMesh->HasTextureCoords(0)) {
                    aiVector3D uv = aMesh->mTextureCoords[0][i];
                    // Flip V to match OpenGL texture coordinate origin if exporter uses opposite origin
                    vertices.push_back(uv.x);
                    vertices.push_back(1.0f - uv.y);
                } else {
                    vertices.push_back(0.0f);
                    vertices.push_back(0.0f);
                }
            }

            std::vector<unsigned int> indices;
            indices.reserve(aMesh->mNumFaces * 3);
            for (unsigned int f = 0; f < aMesh->mNumFaces; ++f)
            {
                const aiFace& face = aMesh->mFaces[f];
                if (face.mNumIndices != 3) continue; // skip non-triangle
                indices.push_back(face.mIndices[0]);
                indices.push_back(face.mIndices[1]);
                indices.push_back(face.mIndices[2]);
            }

            auto mesh = Mesh::create(vertices, indices);

            // Material / texture handling
            std::shared_ptr<Texture> albedo_tex = nullptr;
            std::shared_ptr<Texture> normal_tex = nullptr;

            if (scene->mMaterials && aMesh->mMaterialIndex < scene->mNumMaterials) {
                aiMaterial* material = scene->mMaterials[aMesh->mMaterialIndex];
                aiString texPath;

                // Diffuse / Albedo texture
                if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS) {
                    std::string tex_rel = texPath.C_Str();
                    // embedded textures are referenced like "*0"; skip embedded for now
                    if (!tex_rel.empty() && tex_rel[0] != '*') {
                        std::filesystem::path full = model_dir / tex_rel;
                        if (std::filesystem::exists(full)) {
                            albedo_tex = std::make_shared<Texture>(full);
                            albedo_tex->load();
                            LOG_INIT_COUT();
                            log(LOG_INFO) << "AssimpLoader: loaded albedo texture: " << full << "\n";
                        }
                        else {
                            LOG_INIT_COUT();
                            log(LOG_WARN) << "AssimpLoader: albedo texture referenced but not found: " << full << "\n";
                        }
                    }
                }

                // Normal map: try NORMALS, then HEIGHT
                if (material->GetTexture(aiTextureType_NORMALS, 0, &texPath) == AI_SUCCESS ||
                    material->GetTexture(aiTextureType_HEIGHT, 0, &texPath) == AI_SUCCESS) {
                    std::string tex_rel = texPath.C_Str();
                    if (!tex_rel.empty() && tex_rel[0] != '*') {
                        std::filesystem::path full = model_dir / tex_rel;
                        if (std::filesystem::exists(full)) {
                            normal_tex = std::make_shared<Texture>(full);
                            normal_tex->load();
                            LOG_INIT_COUT();
                            log(LOG_INFO) << "AssimpLoader: loaded normal texture: " << full << "\n";
                        } else {
                            LOG_INIT_COUT();
                            log(LOG_WARN) << "AssimpLoader: normal texture referenced but not found: " << full << "\n";
                        }
                    }
                }
            }

            // Fallbacks if textures not found
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
            r.transform = glm::mat4(1.0f);

            out.push_back(std::move(r));
        }

        return out;
    }

}
