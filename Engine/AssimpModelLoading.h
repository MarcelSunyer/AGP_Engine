#ifndef ASSIMP_MODEL_LOADER_H
#define ASSIMP_MODEL_LOADER_H

#include "Code/platform.h"
#include "Code/engine.h"
#include "Code/Structs.hpp"
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <vector>
#include <string>
#include <cstdio>
#include <glm/glm.hpp>
#include <glad/glad.h>

void ProcessAssimpMesh(const aiScene* scene, aiMesh* mesh, Mesh* myMesh, u32 baseMeshMaterialIndex, std::vector<u32>& submeshMaterialIndices);
void ProcessAssimpMaterial(App* app, aiMaterial* material, Material& myMaterial, String directory);
void ProcessAssimpNode(const aiScene* scene, aiNode* node, Mesh* myMesh, u32 baseMeshMaterialIndex, std::vector<u32>& submeshMaterialIndices);
u32 LoadModel(App* app, const char* filename);

String MakeString(const char* str);
String MakePath(String directory, String filename);
u32 LoadTexture2D(App* app, const char* filepath);




#endif // ASSIMP_MODEL_LOADER_H