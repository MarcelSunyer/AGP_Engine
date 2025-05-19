#ifndef ASSIMP_MODEL_LOADER_H
#define ASSIMP_MODEL_LOADER_H

#include "platform.h" 
#include "engine.h"  
#include "Structs.hpp" 
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <vector>
#include <string>
#include <cstdio>
#include <glm/glm.hpp>
#include <glad/glad.h>
#include <unordered_map>

struct App;
struct Mesh;
struct Material;
struct String;
struct VertexBufferLayout;
struct VertexBufferAttribute;
struct Submesh;
struct Model;
struct Texture; 

void ProcessAssimpMesh(const aiScene* scene, aiMesh* mesh, Mesh* myMesh, u32 baseMeshMaterialIndex, std::vector<u32>& submeshMaterialIndices);
void ProcessAssimpMaterial(App* app, aiMaterial* material, Material& myMaterial, String directory);
void ProcessAssimpNode(const aiScene* scene, aiNode* node, Mesh* myMesh, u32 baseMeshMaterialIndex, std::vector<u32>& submeshMaterialIndices);
u32 LoadModel(App* app, const char* filename);

String MakeString(const char* str);
String MakePath(String directory, String filename);
u32 LoadTexture2D(App* app, const char* filepath, TextureType type); 

void ActivateModel(App* app, u32 modelIndex);
void DeactivateModel(App* app, u32 modelIndex);


#endif 