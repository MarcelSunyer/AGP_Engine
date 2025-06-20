﻿#ifndef STRUCTS
#define STRUCTS

#include"platform.h"
#include <glad/glad.h> 
#include <stdexcept>

typedef glm::vec2  vec2;
typedef glm::vec3  vec3;
typedef glm::vec4  vec4;
typedef glm::ivec2 ivec2;
typedef glm::ivec3 ivec3;
typedef glm::ivec4 ivec4;

struct Camera {
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 worldUp;
    glm::quat orientation;
    float movementSpeed;
    float mouseSensitivity;
    bool isRotating;

    glm::mat4 projectionMatrix;
    glm::mat4 viewMatrix;
    
    void updateCameraVectors(Camera* camera) {
        camera->front = glm::normalize(camera->orientation * glm::vec3(0.0f, 0.0f, -1.0f));
        camera->right = glm::normalize(glm::cross(camera->front, camera->worldUp));
        camera->up = glm::normalize(glm::cross(camera->right, camera->front));
    }
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

enum TextureType
{
    Albedo,
    Normal,
    Height
};

struct Texture
{
    GLuint      handle;
    std::string filepath;
    TextureType type;
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
    GLuint handle;
    std::string filepath;
    std::string programName;
    u64 lastWriteTimestamp;
    VertexShaderLayout vertexInputLayout;
};

enum Mode
{
    Mode_Forward_Geometry,
    Mode_Deferred_Geometry,
    Mode_Count
};

struct VertexV3V2
{
    glm::vec3 pos;
    glm::vec2 uv;
};

const VertexV3V2 vertices[] = {

    {   glm::vec3(-1.0,-1.0,0.0), glm::vec2(0.0,0.0)    },  //Bottom-left verex
    {   glm::vec3(1.0,-1.0,0.0),  glm::vec2(1.0,0.0)    },  //Bottom-Right vertex
    {   glm::vec3(1.0,1.0,0.0),   glm::vec2(1.0,1.0)    },  //Top_Right vertex
    {   glm::vec3(-1.0,1.0,0.0),  glm::vec2(0.0,1.0)    },  //Top_Left vertex
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

struct VertexV3V2N3T3 {
    glm::vec3 pos;
    glm::vec2 uv;
    glm::vec3 normal;
    glm::vec3 tangent;
};

struct VertexBufferLayout {
    std::vector<VertexBufferAttribute> attributes;
    u8 stride;
};

enum EntityType
{
    Deferred_Rendering,
    Relief_Mapping,
    Enviroment_Map,
};

struct Entity {

    glm::mat4 worldMatrix;
    u32 modelIndex;
    u32 entityBufferOffset;
    u32 entityBufferSize;
    std::string name;
    bool active;
    EntityType type;
};


struct Model {
    u32 meshIdx;
    std::vector<u32> materialIdx;
};

struct Vao
{
    GLuint handle;
    GLuint programHandle;
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
    u32 normalsTextureIdx;
    u32 heighTextureIdx;
};

enum class LightType {
    Light_Directional,
    Light_Point,

};

struct Light
{
    LightType type;
    vec3 color;
    vec3 direction;
    vec3 position;
    float intensity;
    int mode;
};

struct FrameBuffer
{
    u32 handle;
    vec2 bufferSize;
    std::vector<std::pair<GLenum, GLuint>> attachments;
    GLuint depthHandle;

    uint64_t _width;
    uint64_t _height;

    bool CreateFBO(const uint64_t aAttachments, const uint64_t aWidth, const uint64_t aHeight)
    {
        _width = aWidth;
        _height = aHeight;

        if (aAttachments > GL_MAX_COLOR_ATTACHMENTS)
        {
            return false;
        }

        std::vector<GLenum> enums;
        for (size_t i = 0; i < aAttachments; ++i)
        {
            GLuint colorAttachment;
            glGenTextures(1, &colorAttachment);
            glBindTexture(GL_TEXTURE_2D, colorAttachment);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, aWidth, aHeight, 0, GL_RGBA, GL_FLOAT, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glBindTexture(GL_TEXTURE_2D, 0);
            attachments.push_back({ GL_COLOR_ATTACHMENT0 + i, colorAttachment });
            enums.push_back(GL_COLOR_ATTACHMENT0 + i);
        }


        glGenTextures(1, &depthHandle);
        glBindTexture(GL_TEXTURE_2D, depthHandle);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, aWidth, aHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);

        glGenFramebuffers(1, &handle);
        glBindFramebuffer(GL_FRAMEBUFFER, handle);

        for (auto it = attachments.cbegin(); it != attachments.cend(); ++it)
        {
            glFramebufferTexture(GL_FRAMEBUFFER, it->first, it->second, 0);
        }

        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthHandle, 0);
        attachments.push_back({ GL_DEPTH_ATTACHMENT, depthHandle });

        GLenum frameBufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);

        if (frameBufferStatus != GL_FRAMEBUFFER_COMPLETE)
        {
            throw std::runtime_error("Framebuffer creation error");
        }

        glDrawBuffers(enums.size(), enums.data());
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void Clear()
    {
        for (auto& texture : attachments)
        {
            glDeleteTextures(1, &texture.second);
            texture.second = 0;
        }
        attachments.clear();
        glDeleteTextures(1, &depthHandle);
        depthHandle = 0;
        glDeleteFramebuffers(1, &handle);
    }

    void Resize(uint64_t width, uint64_t height)
    {
        if (width == _width && height == _height || height == 0 || width == 0)
            return;

        Clear();
        CreateFBO(4, width, height);

    }
};

struct CubeMap
{
    std::vector<std::string> faces1;
    std::vector<std::string> faces2;
    std::vector<std::string> faces3;

    std::vector<unsigned char*> faces1Data;
    std::vector<unsigned char*> faces2Data;
    std::vector<unsigned char*> faces3Data;

    std::vector<std::pair<int, int>> faces1Sizes;
    std::vector<std::pair<int, int>> faces2Sizes;
    std::vector<std::pair<int, int>> faces3Sizes;
    std::vector<float> cubemapCubeVertices = {
    -1.0f,  1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f,  1.0f,
     1.0f,  1.0f,  1.0f
    };

    std::vector<unsigned int> cubemapCubeIndices = {
        0, 1, 2, 2, 3, 0,
        4, 5, 6, 6, 7, 4,
        4, 5, 1, 1, 0, 4,
        3, 2, 6, 6, 7, 3,
        4, 0, 3, 3, 7, 4,
        1, 5, 6, 6, 2, 1 
    };

    GLuint VAO = 0;
    GLuint VBO = 0;
    GLuint EBO = 0;

    u32 cubeMapTexture;
};

struct App
{
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
    u32 forwardProgramIdx;
    u32 reliefMappingIdx;
    u32 environmentMapIdx;
    u32 cubeMapIdx;

    u32 cubemapTexHandle;
    //Modelo 3D cargado
    u32 pikachu;
    u32 patrickTextureUniform;

    //u32 sphereIdx;

    // Mode
    Mode mode;

    // Embedded geometry (in-editor simple meshes such as
    // a screen filling quad, a cube, a sphere...)
    Buffer embeddedVertices;
    GLuint embeddedElements;

    // Location of the texture uniform in the textured quad shader
    GLuint programUniformTexture;

    // VAO object to link our screen filling quad with our textured quad shader
    GLuint vao;

    std::vector<std::string> glExtensions;

    Camera worldCamera;
    GLint maxUniformBufferSize;
    GLint uniformBlockAlignment;

    Buffer entityUBO;
    Buffer globalUBO;
    Buffer localParamsUBO;

    std::vector<Entity> entities;
    std::vector<Entity> spheres;
    std::vector<Light> lights;

    FrameBuffer primaryFBO;

    int attachmentIndex;

    enum BufferViewMode {
        BUFFER_VIEW_MAIN,
        BUFFER_VIEW_ALBEDO,
        BUFFER_VIEW_NORMALS,
        BUFFER_VIEW_POSITION,
        BUFFER_VIEW_VIEWDIR,
        BUFFER_VIEW_DEPTH
    };
    enum ReliefViewMode {
        Relief_VIEW_MAIN,
        Relief_VIEW_NORMALS,
        Relief_VIEW_HEIGHT,
    };

    enum CubeMapViewMode {
        CubeMap_Reflection,
        CubeMap_Refraction,
    };
    BufferViewMode bufferViewMode = BUFFER_VIEW_MAIN;
    
    ReliefViewMode reliefViewMode = Relief_VIEW_MAIN;

    CubeMapViewMode cubemapView = CubeMap_Reflection;

    bool showDepthOverlay = false;
    float reliefIntensity = 0.05f;

    int pgaType;

    bool useForwardRendering; 
    bool isRotating = false;

    CubeMap cubeMap;

    bool gravitationalCamera = false;


    float diffuse = 2.5f;
};

#endif // !STRUCTS
