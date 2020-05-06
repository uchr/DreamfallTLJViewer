#include "MeshExporter.h"
#include "SceneNode.h"

#include <assimp/scene.h>
#include <assimp/Exporter.hpp>
#include <assimp/postprocess.h>

#include <magnum/Math/Matrix4.h>
#include <magnum/Math/Vector3.h>
#include <magnum/Math/Quaternion.h>

#include <iostream>

namespace magnum = Magnum::Math;

aiNode* convertNode(const SceneNode& node, std::vector<aiMesh*>& aMeshes)
{
    aiNode* aNode = new aiNode();

    float scale = 1.0f / node.scale;
    magnum::Vector3 position(node.position.x, node.position.y, node.position.z);
    magnum::Quaternion rotation(magnum::Vector3(node.rotation.x, node.rotation.y, node.rotation.z), node.rotation.w);
    if (rotation.length() < 0.01f)
        rotation = magnum::Quaternion<float>();
    else {
        scale *= rotation.dot();
        rotation = rotation.normalized();
    }
    magnum::Matrix4<float> rotationMatrix = magnum::Matrix4<float>::from(rotation.toMatrix(), {});
    magnum::Matrix4<float> scalingMatrix = magnum::Matrix4<float>::scaling(magnum::Vector3(scale));
    magnum::Matrix4<float> translationMatrix = magnum::Matrix4<float>::translation(position);
    magnum::Matrix4<float> transformation = scalingMatrix * rotationMatrix * translationMatrix;

    aNode->mTransformation = aiMatrix4x4(transformation[0][0], transformation[1][0], transformation[2][0], transformation[3][0],
                                         transformation[0][1], transformation[1][1], transformation[2][1], transformation[3][1],
                                         transformation[0][2], transformation[1][2], transformation[2][2], transformation[3][2],
                                         transformation[0][3], transformation[1][3], transformation[2][3], transformation[3][3]);


    if (node.mesh.has_value()) {
        aMeshes.emplace_back(new aiMesh());
        aiMesh* aMesh = aMeshes[aMeshes.size() - 1];
        aMesh->mMaterialIndex = 0;

        aNode->mMeshes = new unsigned int[1];
        aNode->mMeshes[0] = aMeshes.size() - 1;
        aNode->mNumMeshes = 1;

        const auto& vertices = node.mesh->vertices;
        const auto& uvs = node.mesh->uvs;

        aMesh->mVertices = new aiVector3D[vertices.size()];
        aMesh->mTextureCoords[0] = new aiVector3D[uvs.size()];

        aMesh->mNumVertices = vertices.size();
        aMesh->mNumUVComponents[0] = uvs.size();

        for (size_t i = 0; i < vertices.size(); ++i) {
            const Vector3& v = vertices[i];
            const Vector2& uv = uvs[i];

            aMesh->mVertices[i] = aiVector3D(v.x, v.y, v.z);
            aMesh->mTextureCoords[0][i] = aiVector3D(uv.x, uv.y, 0);
        }

        const auto& indices = node.mesh->indices;

        aMesh->mFaces = new aiFace[indices.size() / 3];
        aMesh->mNumFaces = indices.size() / 3;

        for (size_t ti = 0; ti < indices.size() / 3; ++ti) {
            aiFace& face = aMesh->mFaces[ti];

            face.mIndices = new unsigned int[3];
            face.mNumIndices = 3;

            face.mIndices[0] = indices[ti * 3 + 0];
            face.mIndices[1] = indices[ti * 3 + 1];
            face.mIndices[2] = indices[ti * 3 + 2];
        }
    }

    if (!node.children.empty()) {
        aiNode** aChildren = new aiNode*[node.children.size()];
        for (size_t i = 0; i < node.children.size(); ++i)
            aChildren[i] = convertNode(node.children[i], aMeshes);
        aNode->addChildren(node.children.size(), aChildren);
    }

    return aNode;
}

bool exportScene(const SceneNode& root, const std::filesystem::path& path) {
    std::vector<aiMesh*> aMeshes;
    aiNode* aRoot = convertNode(root, aMeshes);

    aiScene scene;
    scene.mRootNode = aRoot;

    scene.mMaterials = new aiMaterial*[1];
    scene.mNumMaterials = 1;
    scene.mMaterials[0] = new aiMaterial();

    scene.mMeshes = new aiMesh*[aMeshes.size()];
    scene.mNumMeshes = aMeshes.size();

    for (size_t i = 0; i < aMeshes.size(); ++i)
        scene.mMeshes[i] = aMeshes[i];

    Assimp::Exporter exporter;
    std::string extension = path.extension().string();
    extension.erase(0, 1);
    aiReturn res = exporter.Export(&scene, extension, path.string().c_str());
    return res == 0;
}