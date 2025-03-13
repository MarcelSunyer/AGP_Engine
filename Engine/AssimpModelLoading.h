#ifndef ASSIMP_MODEL_LOADER_H
#define ASSIMP_MODEL_LOADER_H

#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <vector>
#include <string>
#include <cstdio>
#include <glm/glm.hpp>
#include <glad/glad.h>
#include "Code/platform.h"

// Type definitions
typedef uint32_t u32;
typedef uint8_t u8;

// Forward declarations
struct App;
struct Mesh;
struct Material;
struct Model;
struct VertexBufferLayout;
struct VertexBufferAttribute;
struct Submesh;
struct String;

// Function declarations
void ProcessAssimpMesh(const aiScene* scene, aiMesh* mesh, Mesh* myMesh, u32 baseMeshMaterialIndex, std::vector<u32>& submeshMaterialIndices);
void ProcessAssimpMaterial(App* app, aiMaterial* material, Material& myMaterial, String directory);
void ProcessAssimpNode(const aiScene* scene, aiNode* node, Mesh* myMesh, u32 baseMeshMaterialIndex, std::vector<u32>& submeshMaterialIndices);
u32 LoadModel(App* app, const char* filename);

// Utility functions
String MakeString(const char* str);
String MakePath(String directory, String filename);
u32 LoadTexture2D(App* app, const char* filepath);

// Structures
struct VertexBufferAttribute {
    u32 location;
    u32 componentCount;
    u32 offset;
};

struct VertexBufferLayout {
    std::vector<VertexBufferAttribute> attributes;
    u32 stride;
};

struct Submesh {
    VertexBufferLayout vertexBufferLayout;
    std::vector<float> vertices;
    std::vector<u32> indices;
    u32 vertexOffset;
    u32 indexOffset;
};

struct Mesh {
    std::vector<Submesh> submeshes;
    GLuint vertexBufferHandle;
    GLuint indexBufferHandle;
};

struct Material {
    std::string name;
    glm::vec3 albedo;
    glm::vec3 emissive;
    float smoothness;
    u32 albedoTextureIdx;
    u32 emissiveTextureIdx;
    u32 specularTextureIdx;
    u32 normalsTextureIdx;
    u32 bumpTextureIdx;
};

struct Model {
    u32 meshIdx;
    std::vector<u32> materialIdx;
};

struct App {
    std::vector<Mesh> meshes;
    std::vector<Material> materials;
    std::vector<Model> models;
};

#endif // ASSIMP_MODEL_LOADER_H