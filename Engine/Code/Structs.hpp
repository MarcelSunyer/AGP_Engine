#ifndef STRUCTS
#define STRUCTS

#include"platform.h"
#include <glad/glad.h>    

typedef glm::vec2  vec2;
typedef glm::vec3  vec3;
typedef glm::vec4  vec4;
typedef glm::ivec2 ivec2;
typedef glm::ivec3 ivec3;
typedef glm::ivec4 ivec4;

struct Camera
{
    glm::mat4 ViewMatrix;
    glm::mat4 ProjectionMatrix;
};

// Buffer structure
struct Buffer {
    u32 size;
    GLenum type;
    GLuint handle;
    u32 head;
    u8* data;
};

struct Image
{
    void* pixels;
    ivec2 size;
    i32   nchannels;
    i32   stride;
};

struct Texture
{
    GLuint      handle;
    std::string filepath;
};

struct VertexShaderAttribute {
    u8 location;
    u8 componentCount;
};

struct VertexShaderLayout {
    std::vector<VertexShaderAttribute> attributes;
};

struct Program
{
    GLuint             handle;
    std::string        filepath;
    std::string        programName;
    u64                lastWriteTimestamp; // What is this for?
    VertexShaderLayout vertexInputLayout;
};

enum Mode
{
    Mode_TexturedQuad,
    Mode_Forward_Geometry,
    Mode_Count
};

struct VertexV3V2
{
    glm::vec3 pos;
    glm::vec2 uv;
};

const VertexV3V2 vertices[] = {

    {   glm::vec3(-0.5,-0.5,0.0), glm::vec2(0.0,0.0)    },  //Bottom-left verex
    {   glm::vec3(0.5,-0.5,0.0),  glm::vec2(1.0,0.0)    },  //Bottom-Right vertex
    {   glm::vec3(0.5,0.5,0.0),   glm::vec2(1.0,1.0)    },  //Top_Right vertex
    {   glm::vec3(-0.5,0.5,0.0),  glm::vec2(0.0,1.0)    },  //Top_Left vertex
};

const u16 indices[] = {

    0, 1, 2,
    0, 2, 3
};

// Structures
struct VertexBufferAttribute {
    u8 location;
    u8 componentCount;
    u8 offset;
};

struct VertexBufferLayout {
    std::vector<VertexBufferAttribute> attributes;
    u8 stride;
};

struct Model {
    u32 meshIdx;
    std::vector<u32> materialIdx;
};

struct Vao
{
    GLuint handle;
    GLuint progranHandle;
};

struct Submesh {
    VertexBufferLayout vertexBufferLayout;
    std::vector<float> vertices;
    std::vector<u32> indices;
    u32 vertexOffset;
    u32 indexOffset;
    std::vector<Vao> vaos;
};

struct Mesh {
    std::vector<Submesh> submeshes;
    GLuint vertexBufferHandle;
    GLuint indexBufferHandle;
};

struct Material {
    std::string name;
    vec3 albedo;
    vec3 emissive;
    f32 smoothness;
    u32 albedoTextureIdx;
    u32 emissiveTextureIdx;
    u32 specularTextureIdx;
    u32 normalsTextureIdx;
    u32 bumpTextureIdx;
};

struct App
{
    //Todo: Posar variables en un constructor
    // Loop
    f32  deltaTime;
    bool isRunning;

    // Input
    Input input;

    // Graphics
    char gpuName[64];
    char openGlVersion[64];

    ivec2 displaySize;

    std::vector<Texture>  textures;
    std::vector<Material>  materials;
    std::vector<Mesh>  meshes;
    std::vector<Model>  models;
    std::vector<Program>  programs;


    // program indices
    u32 texturedGeometryProgramIdx;
    u32 geometryProgramIdx;

    //Modelo 3D cargado
    u32 patrickIdx;
    u32 patrickTextureUniform;


    // texture indices
    u32 diceTexIdx;
    u32 whiteTexIdx;
    u32 blackTexIdx;
    u32 normalTexIdx;
    u32 magentaTexIdx;

    // Mode
    Mode mode;

    // Embedded geometry (in-editor simple meshes such as
    // a screen filling quad, a cube, a sphere...)
    GLuint embeddedVertices;
    GLuint embeddedElements;

    // Location of the texture uniform in the textured quad shader
    GLuint programUniformTexture;

    // VAO object to link our screen filling quad with our textured quad shader
    GLuint vao;

    Camera woldCamera;
    GLint maxUniformBufferSize;
    GLint uniformBlockAlignment;
    Buffer localParamsUBO;
};


#endif // !STRUCTS
