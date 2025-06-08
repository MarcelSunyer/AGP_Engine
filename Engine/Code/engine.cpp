//
// engine.cpp : Put all your graphics stuff in this file. This is kind of the graphics module.
// In here, you should type all your OpenGL commands, and you can also type code to handle
// input platform events (e.g to move the camera or react to certain shortcuts), writing some
// graphics related GUI options, and so on.
//

#include "engine.h"

#include "AssimpModelLoading.h"

#include <imgui.h>
#include <stb_image.h>
#include <stb_image_write.h>

#include <iostream>

void CreateEntity(App* app, const u32 aModelIndx, const glm::mat4& aVP, const glm::mat4& aPosition, std::string name, EntityType type)
{
    Entity entity;
    AlignHead(app->entityUBO, app->uniformBlockAlignment);
    entity.entityBufferOffset = app->entityUBO.head;

    entity.worldMatrix = aPosition;
    entity.modelIndex = aModelIndx;
    entity.name = name;
    entity.type = type;
    glm::mat4 normalMatrix = glm::transpose(glm::inverse(aPosition));
    PushMat4(app->entityUBO, entity.worldMatrix);
    PushMat4(app->entityUBO, normalMatrix);
    PushMat4(app->entityUBO, aVP * entity.worldMatrix);

    entity.entityBufferSize = app->entityUBO.size - entity.entityBufferOffset;

    app->entities.push_back(entity);
}
void CreateLight(App* app, LightType light, vec3 color, vec3 position, float intensity, int mode)
{
    glm::mat4 VP = app->worldCamera.projectionMatrix * app->worldCamera.viewMatrix;

    if (light == LightType::Light_Directional)
    {
        app->lights.push_back({ light, color, position, position, intensity , mode});
    }
    else
    {
        vec3 spherePosition = position;
        glm::mat4 sphereWorld = glm::translate(spherePosition);

        //Comentat ja que no me crea mes d'una esfera jiji
        //CreateEntity(app, app->sphereIdx, VP, sphereWorld);

        app->lights.push_back({ light, color, vec3(0), position, intensity, mode });
    }
}


GLuint CreateProgramFromSource(String programSource, const char* shaderName)
{
    GLchar  infoLogBuffer[1024] = {};
    GLsizei infoLogBufferSize = sizeof(infoLogBuffer);
    GLsizei infoLogSize;
    GLint   success;

    char versionString[] = "#version 430\n";
    char shaderNameDefine[128];
    sprintf(shaderNameDefine, "#define %s\n", shaderName);
    char vertexShaderDefine[] = "#define VERTEX\n";
    char fragmentShaderDefine[] = "#define FRAGMENT\n";

    const GLchar* vertexShaderSource[] = {
        versionString,
        shaderNameDefine,
        vertexShaderDefine,
        programSource.str
    };
    const GLint vertexShaderLengths[] = {
        (GLint)strlen(versionString),
        (GLint)strlen(shaderNameDefine),
        (GLint)strlen(vertexShaderDefine),
        (GLint)programSource.len
    };
    const GLchar* fragmentShaderSource[] = {
        versionString,
        shaderNameDefine,
        fragmentShaderDefine,
        programSource.str
    };
    const GLint fragmentShaderLengths[] = {
        (GLint)strlen(versionString),
        (GLint)strlen(shaderNameDefine),
        (GLint)strlen(fragmentShaderDefine),
        (GLint)programSource.len
    };

    GLuint vshader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vshader, ARRAY_COUNT(vertexShaderSource), vertexShaderSource, vertexShaderLengths);
    glCompileShader(vshader);
    glGetShaderiv(vshader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vshader, infoLogBufferSize, &infoLogSize, infoLogBuffer);
        ELOG("glCompileShader() failed with vertex shader %s\nReported message:\n%s\n", shaderName, infoLogBuffer);
    }

    GLuint fshader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fshader, ARRAY_COUNT(fragmentShaderSource), fragmentShaderSource, fragmentShaderLengths);
    glCompileShader(fshader);
    glGetShaderiv(fshader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fshader, infoLogBufferSize, &infoLogSize, infoLogBuffer);
        ELOG("glCompileShader() failed with fragment shader %s\nReported message:\n%s\n", shaderName, infoLogBuffer);
    }

    GLuint programHandle = glCreateProgram();
    glAttachShader(programHandle, vshader);
    glAttachShader(programHandle, fshader);
    glLinkProgram(programHandle);
    glGetProgramiv(programHandle, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(programHandle, infoLogBufferSize, &infoLogSize, infoLogBuffer);
        ELOG("glLinkProgram() failed with program %s\nReported message:\n%s\n", shaderName, infoLogBuffer);
    }

    glUseProgram(0);

    glDetachShader(programHandle, vshader);
    glDetachShader(programHandle, fshader);
    glDeleteShader(vshader);
    glDeleteShader(fshader);

    return programHandle;
}

u32 LoadProgram(App* app, const char* filepath, const char* programName)
{
    String programSource = ReadTextFile(filepath);

    Program program = {};
    program.handle = CreateProgramFromSource(programSource, programName);
    program.filepath = filepath;
    program.programName = programName;
    program.lastWriteTimestamp = GetFileLastWriteTimestamp(filepath);

    if (program.handle != 0)
    {
        GLint AttributeCount = 0UL;
        glGetProgramiv(program.handle, GL_ACTIVE_ATTRIBUTES, &AttributeCount);
        for (size_t i = 0; i < AttributeCount; ++i)
        {
            GLchar Name[248];
            GLsizei realNameSize = 0UL;
            GLsizei attribSize = 0UL;
            GLenum attribType;
            glGetActiveAttrib(program.handle, i, ARRAY_COUNT(Name), &realNameSize, &attribSize, &attribType, Name);
            GLuint attribLocation = glGetAttribLocation(program.handle, Name);
            program.vertexInputLayout.attributes.push_back({ static_cast<u8>(attribLocation), static_cast<u8>(attribSize) });
        }
    }

    app->programs.push_back(program);

    return app->programs.size() - 1;
}

Image LoadImage(const char* filename)
{
    Image img = {};
    stbi_set_flip_vertically_on_load(true);
    img.pixels = stbi_load(filename, &img.size.x, &img.size.y, &img.nchannels, 0);
    if (img.pixels)
    {
        img.stride = img.size.x * img.nchannels;
    }
    else
    {
        ELOG("Could not open file %s", filename);
    }
    return img;
}


GLuint CreateTexture2DFromImage(Image image, TextureType type) {
    GLenum internalFormat = GL_RGB8;
    GLenum dataFormat = GL_RGB;
    GLenum dataType = GL_UNSIGNED_BYTE;

    switch (image.nchannels) {
    case 1:
        dataFormat = GL_RED;
        internalFormat = GL_R8;
        break;
    case 3:
        dataFormat = GL_RGB;
        internalFormat = GL_RGB8;
        break;
    case 4:
        dataFormat = GL_RGBA;
        internalFormat = GL_RGBA8;
        break;
    default:
        ELOG("LoadTexture2D() - Unsupported number of channels");
    }

    GLuint texHandle;
    glGenTextures(1, &texHandle);
    glBindTexture(GL_TEXTURE_2D, texHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, image.size.x, image.size.y, 0, dataFormat, dataType, image.pixels);

    // Texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glGenerateMipmap(GL_TEXTURE_2D);

    if (type == TextureType::Normal) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    else if (type == TextureType::Height) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    return texHandle;
}
void FreeImage(Image image)
{
    stbi_image_free(image.pixels);
}

u32 LoadTexture2D(App* app, const char* filepath, TextureType type)
{
    for (u32 texIdx = 0; texIdx < app->textures.size(); ++texIdx)
        if (app->textures[texIdx].filepath == filepath)
            return texIdx;

    Image image = LoadImage(filepath);

    if (image.pixels)
    {
        Texture tex = {};
        tex.handle = CreateTexture2DFromImage(image, type);
        tex.filepath = filepath;
        tex.type = type;

        u32 texIdx = app->textures.size();
        app->textures.push_back(tex);

        FreeImage(image);
        return texIdx;
    }
    else
    {
        return UINT32_MAX;
    }
}

void RenderScreenFillQuad(App* app, const FrameBuffer& aFBO)
{
    // Setup framebuffer and viewport
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, app->displaySize.x, app->displaySize.y);

    // Bind shader and geometry
    Program& program = app->programs[app->texturedGeometryProgramIdx];
    glUseProgram(program.handle);
    glBindVertexArray(app->vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, app->embeddedElements);

    // Bind UBO
    glBindBufferRange(GL_UNIFORM_BUFFER, 0, app->globalUBO.handle, 0, app->globalUBO.size);

    // Bind textures
    const char* textureNames[] = { "uColor", "uNormals", "uPosition", "uViewDir", "uDepth" };
    for (int i = 0; i < 5; ++i) {
        glActiveTexture(GL_TEXTURE0 + i);
        GLuint texHandle = (i < 4) ? aFBO.attachments[i].second : aFBO.depthHandle;
        glBindTexture(GL_TEXTURE_2D, texHandle);
        glUniform1i(glGetUniformLocation(program.handle, textureNames[i]), i);
    }

    // Set rendering parameters
    glUniform1f(glGetUniformLocation(program.handle, "uNear"), 0.1f);
    glUniform1f(glGetUniformLocation(program.handle, "uFar"), 1000.0f);
    glUniform1i(glGetUniformLocation(program.handle, "uViewMode"), static_cast<int>(app->bufferViewMode));
    glUniform1i(glGetUniformLocation(program.handle, "uShowDepth"), app->showDepthOverlay ? 1 : 0);
    glUniform1f(glGetUniformLocation(program.handle, "uHeightScale"), app->reliefIntensity);

    // Render quad
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);

    glBindVertexArray(0);
    glUseProgram(0);
    for (int i = 0; i < 5; ++i) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    glActiveTexture(GL_TEXTURE0);
    UpdateLights(app);
}
void SetUpCamera(App* app) {
    app->worldCamera.position = glm::vec3(12.5f, 200.0f, 160.0f);
    app->worldCamera.worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
    app->worldCamera.movementSpeed = 150.0f;
    app->worldCamera.mouseSensitivity = 0.1f;
    app->worldCamera.isRotating = false;

    glm::vec3 target = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 direction = glm::normalize(target - app->worldCamera.position);

    app->worldCamera.orientation = glm::quatLookAt(direction, app->worldCamera.worldUp);

    UpdateCameraVectors(&app->worldCamera);
}

void InitMeshBuffers(App* app)
{
    glEnable(GL_DEPTH_TEST);
    //VBO Init
    app->embeddedVertices = CreateStaticIndexBuffer(sizeof(vertices));
    glBindBuffer(GL_ARRAY_BUFFER, app->embeddedVertices.handle);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    //EBO Init
    glGenBuffers(1, &app->embeddedElements);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, app->embeddedElements);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    //VAO Init
    // Attribute state
    glGenVertexArrays(1, &app->vao);
    glBindVertexArray(app->vao);
    glBindBuffer(GL_ARRAY_BUFFER, app->embeddedVertices.handle);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexV3V2), (void*)0);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(VertexV3V2), (void*)12);
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, app->embeddedElements);
    glBindVertexArray(0);
}

void ExtensionsOpenGL(App* app)
{
    GLint numExtensions = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);
    app->glExtensions.reserve(numExtensions);
    for (GLint i = 0; i < numExtensions; ++i)
    {
        const char* extension = (const char*)glGetStringi(GL_EXTENSIONS, i);
        app->glExtensions.emplace_back(extension);
    }
}

void TestParaMiquel(App* app)
{
    for (int x = -15; x < 15; ++x)
    {
        for (int z = -15; z < 15; ++z)
        {
            glm::vec3 color = glm::vec3(rand() % 100 / 100.0f, rand() % 100 / 100.0f, rand() % 100 / 100.0f);
            CreateLight(app, LightType::Light_Point, color, glm::vec3(x * 15, 0.0f, z * 15), 2, 1);
        }
    }
}

void SetUpCubeMap(App* app) {
   
    // Configurar geometría del cubemap
    glGenVertexArrays(1, &app->cubeMap.VAO);
    glGenBuffers(1, &app->cubeMap.VBO);
    glGenBuffers(1, &app->cubeMap.EBO);

    glBindVertexArray(app->cubeMap.VAO);

    // Configurar VBO
    glBindBuffer(GL_ARRAY_BUFFER, app->cubeMap.VBO);
    glBufferData(GL_ARRAY_BUFFER,
        app->cubeMap.cubemapCubeVertices.size() * sizeof(float),
        app->cubeMap.cubemapCubeVertices.data(),
        GL_STATIC_DRAW);

    // Configurar EBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, app->cubeMap.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
        app->cubeMap.cubemapCubeIndices.size() * sizeof(unsigned int),
        app->cubeMap.cubemapCubeIndices.data(),
        GL_STATIC_DRAW);

    // Atributos de vértices
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    glBindVertexArray(0);

    glGenTextures(1, &app->cubeMap.cubeMapTexture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, app->cubeMap.cubeMapTexture);
    
    for (unsigned int i = 0; i < 6; i++)
    {
        int width, height, nrChannels;

        unsigned char* data = stbi_load(app->cubeMap.faces1[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            stbi_set_flip_vertically_on_load(false);
            glTexImage2D(
                GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
            );
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Failed Loading Image" << app->cubeMap.faces1[i].c_str() << std::endl;
            stbi_image_free(data);
        }
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glGenerateMipmap(GL_TEXTURE_CUBE_MAP); // Generate mipmaps
}


void InitCubeMaps(App* app)
{
    app->cubeMap.faces1 = {
        "CubeMap/px.png", "CubeMap/nx.png",
        "CubeMap/py.png", "CubeMap/ny.png",
        "CubeMap/pz.png", "CubeMap/nz.png"
    };

    app->cubeMap.faces2 = {
        "CubeMap/px_.png", "CubeMap/nx_.png",
        "CubeMap/py_.png", "CubeMap/ny_.png",
        "CubeMap/pz_.png", "CubeMap/nz_.png"
    };

    app->cubeMap.faces3 = {
        "CubeMap/_px.png", "CubeMap/_nx.png",
        "CubeMap/_py.png", "CubeMap/_ny.png",
        "CubeMap/_pz.png", "CubeMap/_nz.png"
    };

    stbi_set_flip_vertically_on_load(false);

    // Load faces1
    for (int i = 0; i < 6; ++i) {
        int w, h, nc;
        unsigned char* data = stbi_load(app->cubeMap.faces1[i].c_str(), &w, &h, &nc, 0);
        app->cubeMap.faces1Data.push_back(data);
        app->cubeMap.faces1Sizes.emplace_back(w, h);
    }

    // Load faces2
    for (int i = 0; i < 6; ++i) {
        int w, h, nc;
        unsigned char* data = stbi_load(app->cubeMap.faces2[i].c_str(), &w, &h, &nc, 0);
        app->cubeMap.faces2Data.push_back(data);
        app->cubeMap.faces2Sizes.emplace_back(w, h);
    }
    for (int i = 0; i < 6; ++i) {
        int w, h, nc;
        unsigned char* data = stbi_load(app->cubeMap.faces3[i].c_str(), &w, &h, &nc, 0);
        app->cubeMap.faces3Data.push_back(data);
        app->cubeMap.faces3Sizes.emplace_back(w, h);
    }
}


void UpdateCubeMap(App* app, int cubemapType) {
    std::vector<unsigned char*>* dataVec;
    std::vector<std::pair<int, int>>* sizeVec;

    switch (cubemapType) {
    case 1:
        dataVec = &app->cubeMap.faces1Data;
        sizeVec = &app->cubeMap.faces1Sizes;
        break;
    case 2:
        dataVec = &app->cubeMap.faces2Data;
        sizeVec = &app->cubeMap.faces2Sizes;
        break;
    case 3:
        dataVec = &app->cubeMap.faces3Data;
        sizeVec = &app->cubeMap.faces3Sizes;
        break;
    default:
        std::cerr << "Invalid cubemapType: " << cubemapType << std::endl;
        return;
    }

    glBindTexture(GL_TEXTURE_CUBE_MAP, app->cubeMap.cubeMapTexture);
    for (unsigned int i = 0; i < 6; ++i) {
        if ((*dataVec)[i]) {
            int width = (*sizeVec)[i].first;
            int height = (*sizeVec)[i].second;

            glTexImage2D(
                GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, (*dataVec)[i]
            );
        }
    }
}

void Init(App* app)
{
    app->pgaType = 2;

    ExtensionsOpenGL(app);

    SetUpCamera(app);

    InitMeshBuffers(app);

    app->texturedGeometryProgramIdx = LoadProgram(app, "Render_Quad.glsl", "Render_Quad");

    app->reliefMappingIdx = LoadProgram(app, "Relief_Mapping.glsl", "RELIEF_MAPPING");

    app->programUniformTexture = glGetUniformLocation(app->programs[app->texturedGeometryProgramIdx].handle, "uTexture");

    app->cubeMapIdx = LoadProgram(app, "CubeMap.glsl", "CUBEMAP");
    //Reflective Shader
    app->environmentMapIdx = LoadProgram(app, "Reflection_environment.glsl", "REFLECTION_ENVIRONMENT");

    InitCubeMaps(app);

    SetUpCubeMap(app);



    u32 cube = LoadModel(app, "Cube/Cube.obj");

    u32 cube2 = LoadModel(app, "Cube2/Cube.obj");

    u32 cube3 = LoadModel(app, "Cube3/Cube.obj");

    app->pikachu = LoadModel(app, "Pikachu/Pikachu.obj");

    u32 planeIdx = LoadModel(app, "Plane/Plane.obj");

    u32 cone = LoadModel(app, "Cone/Cone.obj");

    u32 torus = LoadModel(app, "Torus/Torus.obj");

    u32 skyBox = LoadModel(app, "SkyBox/SkyBox.obj");

    u32 sphere = LoadModel(app, "Sphere/Sphere.obj");

    u32 monkey = LoadModel(app, "Monkey/Monkey.obj");
     
    u32 car = LoadModel(app, "Car/Car.obj");

    //Comentat ja que no me crea mes d'una esfera jiji
    //app->sphereIdx = LoadModel(app, "Sphere/Sphere_Light.obj");

    u32 test_1 = LoadModel(app, "Test/Entity_test.obj");
    app->forwardProgramIdx = LoadProgram(app, "FORWARD.glsl", "FORWARD");
    app->geometryProgramIdx = LoadProgram(app, "RENDER_GEOMETRY.glsl", "RENDER_GEOMETRY");
    app->patrickTextureUniform = glGetUniformLocation(app->programs[app->geometryProgramIdx].handle, "uTexture");

    float aspectRatio = (float)app->displaySize.x / (float)app->displaySize.y;
    float near = 0.1f;
    float far = 5000.0f;
    app->worldCamera.projectionMatrix = glm::perspective(glm::radians(60.0f), aspectRatio, near, far);
    app->worldCamera.viewMatrix = glm::lookAt(vec3(47, 26, 40), vec3(0, 1, 0), vec3(0, 1, 0));


    glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &app->maxUniformBufferSize);
    glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &app->uniformBlockAlignment);

    // Con esto (tamaño dinámico):
    const u32 lightDataSize = app->lights.size() * (sizeof(int) + 3 * sizeof(vec4));
    app->globalUBO = CreateConstantBuffer(lightDataSize + sizeof(vec4) + sizeof(int));
    app->entityUBO = CreateConstantBuffer(app->maxUniformBufferSize);

    //TestParaMiquel(app);  //Crea 1000 llums a l'escena
    CreateLight(app, LightType::Light_Directional, vec3(1.0), vec3(1, 0, 0), 2, 1);
    CreateLight(app, LightType::Light_Directional, vec3(1.0), vec3(1, 0, 0), 2, 2);
    CreateLight(app, LightType::Light_Directional, vec3(1.0), vec3(0, 0, 1), 2, 2);
    CreateLight(app, LightType::Light_Directional, vec3(1.0), vec3(1, 0, 0), 7, 3);

    Buffer& entityUBO = app->entityUBO;
    MapBuffer(entityUBO, GL_WRITE_ONLY);
    glm::mat4 VP = app->worldCamera.projectionMatrix * app->worldCamera.viewMatrix;

    CreateEntity(app, cube, VP, glm::translate(glm::vec3(70, 0, 0)), "Cube", EntityType::Relief_Mapping);
    
    CreateEntity(app, cube2, VP, glm::translate(glm::vec3(0, 0, 0)), "Cube2", EntityType::Relief_Mapping);

    CreateEntity(app, cube3, VP, glm::translate(glm::vec3(-70, 0, 0)), "Cube3", EntityType::Relief_Mapping);

    CreateEntity(app, cone, VP, glm::translate(glm::vec3(-30, 0, 0)), "Cone", EntityType::Deferred_Rendering);

    CreateEntity(app, torus, VP, glm::translate(glm::vec3(-30, 0, -30)), "Torus", EntityType::Deferred_Rendering);

    CreateEntity(app, sphere, VP, glm::translate(glm::vec3(30, 0, -30)), "Sphere", EntityType::Deferred_Rendering);

    CreateEntity(app, sphere, VP, glm::translate(glm::vec3(-40, 0, 0)), "Sphere", EntityType::Enviroment_Map);
    
    CreateEntity(app, car, VP, glm::translate(glm::vec3(30, 0, 10)), "Car", EntityType::Enviroment_Map);

    CreateEntity(app, monkey, VP, glm::translate(glm::vec3(0, 0, 0)), "Monkey", EntityType::Deferred_Rendering);

    CreateEntity(app, planeIdx, VP, glm::identity<glm::mat4>(), "Plane", EntityType::Deferred_Rendering);

    CreateEntity(app, skyBox, VP, glm::identity<glm::mat4>(), "SkyBox", EntityType::Deferred_Rendering);

    CreateEntity(app, app->pikachu, VP, glm::translate(glm::vec3(0, 0, 0)), "Pikachu", EntityType::Deferred_Rendering);

    UnmapBuffer(entityUBO);

    app->mode = Mode_Deferred_Geometry;
    app->useForwardRendering = false;

    app->primaryFBO.CreateFBO(4, app->displaySize.x, app->displaySize.y);
    UpdateLights(app);
}

// Helper function for the toggle button
static void ToggleButton(const char* str_id, bool* v) {

    ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    float height = ImGui::GetFrameHeight();
    float width = height * 1.55f;
    float radius = height * 0.50f;

    if (ImGui::InvisibleButton(str_id, ImVec2(width, height)))
        *v = !*v;

    ImU32 col_bg = ImGui::IsItemHovered() ?
        (*v ? IM_COL32(165, 231, 88, 255) : IM_COL32(198, 198, 198, 255)) :
        (*v ? IM_COL32(145, 211, 68, 255) : IM_COL32(218, 218, 218, 255));

    draw_list->AddRectFilled(p, ImVec2(p.x + width, p.y + height), col_bg, height * 0.5f);
    draw_list->AddCircleFilled(ImVec2(*v ? (p.x + width - radius) : (p.x + radius), p.y + radius), radius - 1.5f, IM_COL32(255, 255, 255, 255));
}
void Gui(App* app)
{
    ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

    ImGui::Begin("Delivery");
    {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));

        const char* viewLabels[] = { "Deferred Rendering", "Relief Mapping", "Environment Map" };

        {
            ImGuiStyle& style = ImGui::GetStyle();
            float size = ImGui::CalcTextSize(viewLabels[0]).x + style.FramePadding.x * 2.0f;
            float avail = ImGui::GetContentRegionAvail().x;
            float off = (avail - size) * 0.4f;
            if (off > 0.0f)
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + off);
        }

        for (int i = 0; i < 3; i++)
        {
            bool isActive = (static_cast<int>(app->pgaType) == i + 1);
            if (isActive)
            {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.4f, 0.8f, 1.0f));
            }

            if (ImGui::Button(viewLabels[i]))
            {
                app->pgaType = i + 1; // Asigna 1, 2 o 3
            }

            if (isActive)
            {
                ImGui::PopStyleColor();
            }

            if (i < 2) // Añade un poco de espacio entre los botones
            {
                ImGui::SameLine();
            }
        }

        ImGui::PopStyleVar();
    }
    ImGui::End();

    if (app->pgaType == 1)
    {
        ImGui::Begin("Viewport");
        {
            ImVec2 viewportSize = ImGui::GetContentRegionAvail();

            GLuint textureID = app->primaryFBO.attachments[0].second;
            switch (app->bufferViewMode) {
            case App::BUFFER_VIEW_ALBEDO:
                textureID = app->primaryFBO.attachments[0].second; break;
            case App::BUFFER_VIEW_NORMALS:
                textureID = app->primaryFBO.attachments[1].second; break;
            case App::BUFFER_VIEW_POSITION:
                textureID = app->primaryFBO.attachments[2].second; break;
            case App::BUFFER_VIEW_VIEWDIR:
                textureID = app->primaryFBO.attachments[3].second; break;
            case App::BUFFER_VIEW_DEPTH:
                textureID = app->primaryFBO.depthHandle; break;
            default:
                break;
            }

            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));

            const char* viewLabels[] = { "Main", "Albedo", "Normals", "Position", "ViewDir", "Depth" };
            constexpr int buttonCount = 6;
            constexpr float buttonWidth = 80.0f;
            constexpr float buttonHeight = 30.0f;
            constexpr float spacing = 8.0f;

            float totalWidth = buttonCount * buttonWidth + (buttonCount - 1) * spacing;
            float availWidth = ImGui::GetContentRegionAvail().x;
            float offsetX = (availWidth - totalWidth) * 0.5f;
            if (offsetX > 0.0f)
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offsetX);

            for (int i = 0; i < buttonCount; ++i) {
                bool isActive = (static_cast<int>(app->bufferViewMode) == i);
                if (isActive) {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.4f, 0.8f, 1.0f));
                }

                if (ImGui::Button(viewLabels[i], ImVec2(buttonWidth, buttonHeight))) {
                    app->bufferViewMode = static_cast<App::BufferViewMode>(i);
                    if (i != 0)
                        app->showDepthOverlay = false;
                }

                if (isActive) {
                    ImGui::PopStyleColor();
                }

                if (i < buttonCount - 1) {
                    ImGui::SameLine(0.0f, spacing);
                }
            }

            ImGui::PopStyleVar();
        }
        ImGui::End();

    }
    if (app->pgaType == 2)
    {
        ImGui::Begin("ViewMode");
        {
            constexpr float buttonWidth = 80.0f;
            constexpr float buttonHeight = 30.0f;
            constexpr float spacing = 8.0f;

            float totalWidth = 3 * buttonWidth + (3 - 1) * spacing;
            float availWidth = ImGui::GetContentRegionAvail().x;
            float offsetX = (availWidth - totalWidth) * 0.5f;
            if (offsetX > 0.0f)
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offsetX);
            if (ImGui::Button("Main", ImVec2(80.0f, 30.0f)))
            {
                app->reliefViewMode = App::Relief_VIEW_MAIN;
            }
            ImGui::SameLine();
            if (ImGui::Button("Normal", ImVec2(80.0f, 30.0f)))
            {
                app->reliefViewMode = App::Relief_VIEW_NORMALS;
            }
            ImGui::SameLine();
            if (ImGui::Button("Height", ImVec2(80.0f, 30.0f)))
            {
                app->reliefViewMode = App::Relief_VIEW_HEIGHT;
            }
        }
    }
    if (app->pgaType == 3)
    {
        ImGui::Begin("CubeMap Render");
        {
            constexpr float buttonWidth = 80.0f;
            constexpr float buttonHeight = 30.0f;
            constexpr float spacing = 8.0f;

            float totalWidth = 3 * buttonWidth + (3 - 1) * spacing;
            float availWidth = ImGui::GetContentRegionAvail().x;
            float offsetX = (availWidth - totalWidth) * 0.5f;
            if (offsetX > 0.0f)
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offsetX);
            if (ImGui::Button("Reflection", ImVec2(80.0f, 30.0f)))
            {
                app->cubemapView = App::CubeMap_Reflection;
            }
            ImGui::SameLine();
            if (ImGui::Button("Refraction", ImVec2(80.0f, 30.0f)))
            {
                app->cubemapView = App::CubeMap_Refraction;
            }         
        }
    }



    ImGui::End();

    ImGui::Begin("Inspector");
    {
        if (ImGui::CollapsingHeader("Entities", ImGuiTreeNodeFlags_DefaultOpen)) {
            bool entityChanged = false;
            glm::mat4 VP = app->worldCamera.projectionMatrix * app->worldCamera.viewMatrix;

            for (size_t i = 0; i < app->entities.size(); ++i) {
                ImGui::PushID(static_cast<int>(i));
                app->entities[6].active == false;
                glm::vec3 entityPosition = glm::vec3(app->entities[i].worldMatrix[3]);

                if (app->entities[i].name == " " || app->entities[i].name == "SkyBox") {
                    //No registra las entities
                }
                else {
                    if (app->entities[i].type == EntityType::Deferred_Rendering && app->pgaType == 1)
                    {
                        std::string label = "Geometry: " + app->entities[i].name;
                        if (ImGui::DragFloat3(label.c_str(), &entityPosition[0], 0.1f)) {
                            app->entities[i].worldMatrix = glm::translate(glm::mat4(1.0f), entityPosition);
                            entityChanged = true;

                        }
                    }
                    else if (app->entities[i].type == EntityType::Relief_Mapping && app->pgaType == 2)
                    {
                        std::string label = "Geometry: " + app->entities[i].name;
                        if (ImGui::DragFloat3(label.c_str(), &entityPosition[0], 0.1f)) {
                            app->entities[i].worldMatrix = glm::translate(glm::mat4(1.0f), entityPosition);
                            entityChanged = true;
                        }
                    }
                    else if (app->entities[i].type == EntityType::Enviroment_Map && app->pgaType == 3)
                    {
                        std::string label = "Geometry: " + app->entities[i].name;
                        if (ImGui::DragFloat3(label.c_str(), &entityPosition[0], 0.1f)) {
                            app->entities[i].worldMatrix = glm::translate(glm::mat4(1.0f), entityPosition);
                            entityChanged = true;
                        }
                    }

                }

                ImGui::PopID();
            }

            if (entityChanged) {
                MapBuffer(app->entityUBO, GL_WRITE_ONLY);
                for (auto& entity : app->entities) {
                    glm::mat4 newVPMatrix = VP * entity.worldMatrix;
                    memcpy((char*)app->entityUBO.data + entity.entityBufferOffset, &entity.worldMatrix, sizeof(glm::mat4));
                    memcpy((char*)app->entityUBO.data + entity.entityBufferOffset + sizeof(glm::mat4), &newVPMatrix, sizeof(glm::mat4));
                }
                UnmapBuffer(app->entityUBO);
            }
        }

        if (app->pgaType == 2)
        {
            ImGui::Begin("Relief Intensity");
            {


                ImGui::SliderFloat("Relief Intensity", &app->reliefIntensity, 0.0f, 0.2f, "%.3f");
                ToggleButton("Rotation", &app->isRotating);
            }

        }
        ImGui::End();
        if (ImGui::CollapsingHeader("Important Info", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Text("FPS: %.1f", 1.0f / app->deltaTime);


            if (app->pgaType == 3) {
                if (ImGui::Begin("CubeMap"))
                {
                    if (ImGui::Button("Outdoor area"))
                    {
                        UpdateCubeMap(app, 1);
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("House"))
                    {
                        UpdateCubeMap(app, 2);
                    }
                    if (ImGui::Button("Studio"))
                    {
                        UpdateCubeMap(app, 3);
                    }
                
                    ImGui::SliderFloat("Diffuse Amb Intensity", &app->diffuse, 0.0f, 5.0f, "%.3f");
                } 
                ImGui::End();

            }
        }
        ImGui::Separator();

        if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::DragFloat3("Position", &app->worldCamera.position[0], 0.1f);

            ImGui::Separator();
            ImGui::Text("Rotation (Quaternion)");

            // Extraer ángulos de Euler del cuaternión para mostrarlos en la UI (solo para visualización/control)
            glm::vec3 eulerAngles = glm::degrees(glm::eulerAngles(app->worldCamera.orientation));

            // Mostrar los ángulos de Euler como controles deslizantes
            float yaw = eulerAngles.y;
            float pitch = eulerAngles.x;
            if (ImGui::DragFloat("Yaw", &yaw, 1.0f, -360.0f, 360.0f)) {
                // Reconstruir el cuaternión con los nuevos ángulos
                app->worldCamera.orientation = glm::quat(glm::vec3(glm::radians(pitch), glm::radians(yaw), 0.0f));
                UpdateCameraVectors(&app->worldCamera);
            }
            if (ImGui::DragFloat("Pitch", &pitch, 1.0f, -89.0f, 89.0f)) {
                // Reconstruir el cuaternión con los nuevos ángulos
                app->worldCamera.orientation = glm::quat(glm::vec3(glm::radians(pitch), glm::radians(yaw), 0.0f));
                UpdateCameraVectors(&app->worldCamera);
            }

            ImGui::Separator();
            ImGui::DragFloat("Speed", &app->worldCamera.movementSpeed, 1, .10f, 300.0f);
            ImGui::DragFloat("Sensitivity", &app->worldCamera.mouseSensitivity, 0.01f, 0.01f, 1.0f);
            ImGui::Checkbox("Is Rotating", &app->worldCamera.isRotating);
        }


        ImGui::Begin("Lights");

        if (ImGui::CollapsingHeader("Lights", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImGui::Button("Add Directional Light")) {
                CreateLight(app, LightType::Light_Directional, vec3(1), vec3(1), 1.f, app->pgaType);
                UpdateLights(app);
            }

            if (ImGui::Button("Add Point Light")) {
                CreateLight(app, LightType::Light_Point, vec3(1), vec3(0), 1.f, app->pgaType);
                UpdateLights(app);
            }
            if (app->pgaType == 1)
            {

                if (ImGui::Button("Add 400 Point Lights"))
                {
                    for (int x = -10; x < 10; ++x)
                    {
                        for (int z = -10; z < 10; ++z)
                        {
                            glm::vec3 color = glm::vec3(rand() % 100 / 100.0f, rand() % 100 / 100.0f, rand() % 100 / 100.0f);
                            CreateLight(app, LightType::Light_Point, color, glm::vec3(x * 50, 0.0f, z * 50), 1, app->pgaType);
                        }
                    }
                    UpdateLights(app);
                }



                if (ImGui::Button("Delete Last 400 lights"))
                {
                    if (!app->lights.empty()) {

                        size_t lightsToRemove = glm::min(app->lights.size(), (size_t)401);

                        app->lights.erase(app->lights.end() - lightsToRemove, app->lights.end());

                        UpdateLights(app);
                    }
                }
            }

            for (size_t i = 0; i < app->lights.size(); ++i)
            {
                ImGui::PushID(static_cast<int>(i));
                Light& light = app->lights[i];
                if (light.mode != app->pgaType)
                {
                    continue;
                }
                bool lightChanged = false;

                ImGui::Separator();

                ImGui::Text("Light %d", i + 1);
                float intensity = light.intensity;

                const char* lightTypes[] = { "Directional", "Point" };

                int currentType = static_cast<int>(light.type);
                if (ImGui::Combo("Type", &currentType, lightTypes, IM_ARRAYSIZE(lightTypes)))
                {
                    light.type = static_cast<LightType>(currentType);
                    lightChanged = true;
                }

                float color[3] = { light.color[0], light.color[1], light.color[2] };
                if (ImGui::ColorEdit3("Color", color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR))
                {
                    light.color = vec3(color[0], color[1], color[2]);
                    lightChanged = true;
                }

                if (light.type == LightType::Light_Directional)
                {
                    float direction[3] = { light.direction.x, light.direction.y, light.direction.z };
                    if (ImGui::DragFloat3("Direction", direction, 0.01f, -1.0f, 1.0f))
                    {
                        light.direction = glm::normalize(vec3(direction[0], direction[1], direction[2]));
                        lightChanged = true;
                    }
                }
                else
                {
                    float position[3] = { light.position.x, light.position.y, light.position.z };
                    if (ImGui::DragFloat3("Position", position, 0.1f))
                    {
                        light.position = vec3(position[0], position[1], position[2]);

                        lightChanged = true;
                    }
                }
                if (ImGui::SliderFloat("Intensity", &intensity, 0.0f, 100.0f, "%.001f"))
                {
                    light.intensity = intensity;
                    lightChanged = true;
                }

                if (ImGui::Button("Delete"))
                {
                    app->lights.erase(app->lights.begin() + i);
                    UpdateLights(app);
                    ImGui::PopID();
                    continue;
                }


                if (lightChanged)
                {
                    UpdateLights(app);
                }

                ImGui::PopID();
            }
            ImGui::End();
        }
        ImGui::Separator();

        ImGui::Begin("OpenGl Info");
        ImGui::Text("Extensions");
        for (const auto& ext : app->glExtensions)
        {
            ImGui::Text("%s", ext.c_str());
        }
        ImGui::End();


        if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::DragFloat3("Position", &app->worldCamera.position[0], 0.1f);

            // Añade esto:
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Gravitational Camera (F)");
            ImGui::SameLine();
            ToggleButton("##GravCamToggle", &app->gravitationalCamera);

        }
        ImGui::End();
        ImGui::Begin("Rendering Mode");
        {
            bool* p_state = &app->useForwardRendering;

            ImGui::Text("Deferred");
            ImGui::SameLine();

            // Toggle switch visual
            ImVec2 toggle_size = ImVec2(40, 20);
            ImVec2 pos = ImGui::GetCursorScreenPos();
            ImDrawList* draw_list = ImGui::GetWindowDrawList();

            // Background and knob colors
            ImU32 bg_color = *p_state ? IM_COL32(100, 200, 100, 255) : IM_COL32(100, 100, 100, 255);
            ImU32 knob_color = IM_COL32(255, 255, 255, 255);

            // Toggle background
            draw_list->AddRectFilled(pos, ImVec2(pos.x + toggle_size.x, pos.y + toggle_size.y),
                bg_color, toggle_size.y * 0.5f);

            // Knob
            float knob_radius = toggle_size.y * 0.4f;
            float knob_x = *p_state ? (pos.x + toggle_size.x - knob_radius - 4)
                : (pos.x + knob_radius + 4);
            draw_list->AddCircleFilled(ImVec2(knob_x, pos.y + toggle_size.y / 2),
                knob_radius, knob_color);

            // Click interaction
            ImGui::InvisibleButton("##RenderingToggle", toggle_size);
            if (ImGui::IsItemClicked()) {
                *p_state = !*p_state;
            }

            ImGui::SameLine();
            ImGui::Text("Forward");
        }
        ImGui::End();

    }
}

void UpdateCameraVectors(Camera* camera) {
    // Calcular los vectores front, right y up a partir del cuaternión de orientación
    camera->front = glm::normalize(camera->orientation * glm::vec3(0.0f, 0.0f, -1.0f));
    camera->right = glm::normalize(glm::cross(camera->front, camera->worldUp));
    camera->up = glm::normalize(glm::cross(camera->right, camera->front));
}

void ProcessMouseMovement(Camera* camera, float xoffset, float yoffset) {
    xoffset *= camera->mouseSensitivity;
    yoffset *= camera->mouseSensitivity;

    // Crear cuaterniones para rotación horizontal (yaw) y vertical (pitch)
    glm::quat yawQuat = glm::angleAxis(glm::radians(-xoffset), camera->worldUp);
    glm::quat pitchQuat = glm::angleAxis(glm::radians(-yoffset), camera->right);

    // Combinar las rotaciones y normalizar
    camera->orientation = glm::normalize(yawQuat * pitchQuat * camera->orientation);

    // Actualizar los vectores de la cámara
    UpdateCameraVectors(camera);
}

void CheckAndReloadShaders(App* app)
{
    for (auto& program : app->programs)
    {
        u64 currentTimestamp = GetFileLastWriteTimestamp(program.filepath.c_str());
        if (currentTimestamp != program.lastWriteTimestamp)
        {
            String programSource = ReadTextFile(program.filepath.c_str());
            GLuint newHandle = CreateProgramFromSource(programSource, program.programName.c_str());

            if (newHandle != 0)
            {
                glDeleteProgram(program.handle);

                program.handle = newHandle;
                program.lastWriteTimestamp = currentTimestamp;


                program.vertexInputLayout.attributes.clear();
                GLint attributeCount = 0;
                glGetProgramiv(program.handle, GL_ACTIVE_ATTRIBUTES, &attributeCount);
                for (size_t i = 0; i < attributeCount; ++i)
                {
                    GLchar name[248];
                    GLsizei realNameSize = 0;
                    GLsizei attribSize = 0;
                    GLenum attribType;
                    glGetActiveAttrib(program.handle, i, ARRAY_COUNT(name), &realNameSize, &attribSize, &attribType, name);
                    GLuint attribLocation = glGetAttribLocation(program.handle, name);
                    program.vertexInputLayout.attributes.push_back({ static_cast<u8>(attribLocation), static_cast<u8>(attribSize) });
                }

                ELOG("Reloaded shader: %s", program.filepath.c_str());
            }
        }
    }
}

void ActiveEntities(App* app, Entity* entity)
{
    if (app->pgaType == 1 && entity->type == EntityType::Deferred_Rendering)
    {
        entity->active = true;
    }

    if (app->pgaType == 2 && entity->type == EntityType::Relief_Mapping)
    {
        entity->active = true;
    }

    if (app->pgaType == 3 && entity->type == EntityType::Enviroment_Map)
    {
        entity->active = true;
    }
}

void DisableEntities(App* app, Entity* entity)
{
    if (app->pgaType == 1 && entity->type != EntityType::Deferred_Rendering)
    {
        entity->active = false;
    }

    if (app->pgaType == 2 && entity->type != EntityType::Relief_Mapping)
    {
        entity->active = false;
    }
    if (app->pgaType == 3 && entity->type != EntityType::Enviroment_Map)
    {
        entity->active = false;
    }
}

void Update(App* app) {

    app->mode = app->useForwardRendering ? Mode_Forward_Geometry : Mode_Deferred_Geometry;

    CheckAndReloadShaders(app);

    if (app->input.mouseButtons[LEFT] == BUTTON_PRESS) {
        app->worldCamera.isRotating = true;
    }
    if (app->input.mouseButtons[LEFT] == BUTTON_RELEASE) {
        app->worldCamera.isRotating = false;
    }

    static bool isPanning = false;
    if (app->input.mouseButtons[RIGHT] == BUTTON_PRESS) {
        isPanning = true;
    }
    if (app->input.mouseButtons[RIGHT] == BUTTON_RELEASE) {
        isPanning = false;
    }

    if (app->worldCamera.isRotating) {
        ProcessMouseMovement(&app->worldCamera,
            app->input.mouseDelta.x,
            app->input.mouseDelta.y);
    }
    else if (isPanning) {
        float panSpeed = 0.005f;
        app->worldCamera.position -= app->worldCamera.right * (app->input.mouseDelta.x * panSpeed);
        app->worldCamera.position += app->worldCamera.up * (app->input.mouseDelta.y * panSpeed);
    }

    float velocity = app->worldCamera.movementSpeed * app->deltaTime;

    if (app->input.keys[K_W] == BUTTON_PRESSED)
    {
        app->worldCamera.position += app->worldCamera.front * velocity;
    }
    if (app->input.keys[K_S] == BUTTON_PRESSED)
    {
        app->worldCamera.position -= app->worldCamera.front * velocity;
    }
    if (app->input.keys[K_A] == BUTTON_PRESSED)
    {
        app->worldCamera.position -= app->worldCamera.right * velocity;
    }
    if (app->input.keys[K_D] == BUTTON_PRESSED)
    {
        app->worldCamera.position += app->worldCamera.right * velocity;
    }
    if (app->input.keys[K_E] == BUTTON_PRESSED)
    {
        app->worldCamera.position += app->worldCamera.up * velocity;
    }
    if (app->input.keys[K_Q] == BUTTON_PRESSED)
    {
        app->worldCamera.position -= app->worldCamera.up * velocity;
    }

    app->worldCamera.viewMatrix = glm::lookAt(app->worldCamera.position, app->worldCamera.position + app->worldCamera.front, app->worldCamera.up);

    glm::mat4 VP = app->worldCamera.projectionMatrix * app->worldCamera.viewMatrix;

    MapBuffer(app->entityUBO, GL_WRITE_ONLY);
    static float animationTime = 0.0f;
    animationTime += app->deltaTime;

    for (auto& entity : app->entities) {
        if (entity.modelIndex == app->pikachu) {
            float rotationAngle = animationTime / 2;
            glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), rotationAngle, glm::vec3(0, 1, 0));

            glm::vec3 pos = glm::vec3(entity.worldMatrix[3]);
            glm::mat4 translation = glm::translate(glm::mat4(1.0f), pos);

            entity.worldMatrix = translation * rotation;
        }
        else if (entity.type == EntityType::Relief_Mapping && app->isRotating ) {
            float rotationAngle = animationTime / 2;
            glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), rotationAngle, glm::vec3(0, 1, 0));

            glm::vec3 pos = glm::vec3(entity.worldMatrix[3]);
            glm::mat4 translation = glm::translate(glm::mat4(1.0f), pos);

            entity.worldMatrix = translation * rotation;
        }

        ActiveEntities(app, &entity);

        DisableEntities(app, &entity);

        glm::mat4 vpMatrix = VP * entity.worldMatrix;
        glm::mat4 normalMatrix = glm::transpose(glm::inverse(entity.worldMatrix));

        memcpy((char*)app->entityUBO.data + entity.entityBufferOffset, &entity.worldMatrix, sizeof(glm::mat4));

        memcpy((char*)app->entityUBO.data + entity.entityBufferOffset + sizeof(glm::mat4), &vpMatrix, sizeof(glm::mat4));

        memcpy((char*)app->entityUBO.data + entity.entityBufferOffset + 2 * sizeof(glm::mat4), &normalMatrix, sizeof(glm::mat4));
    }

    if (app->input.keys[K_F] == BUTTON_PRESSED) {
        app->gravitationalCamera = !app->gravitationalCamera;
    }

    // Mantener orientación si está activado
    if (app->gravitationalCamera) {
        glm::vec3 direction = glm::normalize(glm::vec3(0) - app->worldCamera.position);

        // Calcular la orientación necesaria para mirar al origen usando cuaterniones
        glm::quat targetOrientation = glm::quatLookAt(direction, app->worldCamera.worldUp);

        // Interpolar suavemente hacia la nueva orientación (opcional)
        app->worldCamera.orientation = glm::slerp(app->worldCamera.orientation, targetOrientation, 0.1f);

        UpdateCameraVectors(&app->worldCamera);

        // Bloquear controles manuales
        app->worldCamera.isRotating = false;
        isPanning = false;
    }

    if (app->gravitationalCamera) {
        app->worldCamera.isRotating = false;
        isPanning = false;
    }

    UnmapBuffer(app->entityUBO);
    UpdateLights(app);
}

void UpdateLights(App* app) {
    // Contar solo las luces activas del modo actual
    u32 activeLightCount = 0;
    for (const auto& light : app->lights) {
        if (light.mode == app->pgaType) {
            ++activeLightCount;
        }
    }

    const u32 sizePerLight = sizeof(int) + 3 * sizeof(vec4); // type + color + dir + pos
    const u32 otherDataSize = sizeof(glm::vec3) + sizeof(int); // cam pos + light count
    const u32 requiredSize = otherDataSize + activeLightCount * sizePerLight;

    // Redimensionar si hace falta
    if (requiredSize > app->maxUniformBufferSize || app->globalUBO.size < requiredSize) {
        UnmapBuffer(app->globalUBO);
        app->globalUBO = CreateConstantBuffer(requiredSize);
    }

    MapBuffer(app->globalUBO, GL_WRITE_ONLY);
    //BindBuffer(app->globalUBO);

    // Subir posición de cámara y número de luces activas
    PushVec3(app->globalUBO, app->worldCamera.position);
    PushUInt(app->globalUBO, activeLightCount);

    for (auto& light : app->lights) {
        if (light.mode != app->pgaType) continue;

        AlignHead(app->globalUBO, 16);
        PushUInt(app->globalUBO, static_cast<int>(light.type));
        PushVec3(app->globalUBO, light.color * light.intensity);
        AlignHead(app->globalUBO, 16);
        PushVec3(app->globalUBO, light.direction);
        AlignHead(app->globalUBO, 16);
        PushVec3(app->globalUBO, light.position);
        PushFloat(app->globalUBO, light.intensity);
    }

    UnmapBuffer(app->globalUBO);
}

void RenderCubeMap(App* app) {

    if (app->pgaType != 3)
    {
        return;
    }

    // Guardar estado actual
    GLint lastProgram;
    GLboolean depthMaskEnabled;
    GLint depthFunc;
    glGetIntegerv(GL_CURRENT_PROGRAM, &lastProgram);
    glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMaskEnabled);
    glGetIntegerv(GL_DEPTH_FUNC, &depthFunc);
    GLint cullFace = glIsEnabled(GL_CULL_FACE);

    // Configurar estado para skybox
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);  // No escribir en depth buffer
    glDisable(GL_CULL_FACE);

    Program& cubeMapProgram = app->programs[app->cubeMapIdx];
    glUseProgram(cubeMapProgram.handle);

    // Eliminar componente de traslación
    glm::mat4 view = glm::mat4(glm::mat3(app->worldCamera.viewMatrix));

    glUniformMatrix4fv(glGetUniformLocation(cubeMapProgram.handle, "uView"), 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(cubeMapProgram.handle, "uProj"), 1, GL_FALSE, &app->worldCamera.projectionMatrix[0][0]);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, app->cubeMap.cubeMapTexture);
    glUniform1i(glGetUniformLocation(cubeMapProgram.handle, "skybox"), 0);

    glBindVertexArray(app->cubeMap.VAO);
    glDrawElements(GL_TRIANGLES,
        static_cast<GLsizei>(app->cubeMap.cubemapCubeIndices.size()),
        GL_UNSIGNED_INT,
        nullptr);

    // Restaurar estado
    glUseProgram(lastProgram);
    glDepthMask(depthMaskEnabled);
    glDepthFunc(depthFunc);
    if (cullFace) glEnable(GL_CULL_FACE);
    else glDisable(GL_CULL_FACE);
}

void Render(App* app)
{
    switch (app->mode)
    {
    case Mode_Forward_Geometry:
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        // 1. Render skybox only for environment mapping mode
        if (app->pgaType == 3) {
            glDepthMask(GL_FALSE);
            RenderCubeMap(app);
            glDepthMask(GL_TRUE);
        }

        // 2. Renderizar otros objetos normalmente
        Program& forwardProgram = app->programs[app->forwardProgramIdx];
        glUseProgram(forwardProgram.handle);

        // Bind global UBO
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, app->globalUBO.handle);

        // Set common matrices and properties
        glm::mat4 view = app->worldCamera.viewMatrix;
        glm::mat4 proj = app->worldCamera.projectionMatrix;
        glUniformMatrix4fv(glGetUniformLocation(forwardProgram.handle, "uView"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(forwardProgram.handle, "uProj"), 1, GL_FALSE, glm::value_ptr(proj));
        glUniform3fv(glGetUniformLocation(forwardProgram.handle, "uCameraPosition"), 1, glm::value_ptr(app->worldCamera.position));

        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_CUBE_MAP, app->cubeMap.cubeMapTexture);
        glUniform1i(glGetUniformLocation(forwardProgram.handle, "uSkybox"), 3);

        // Reset material flags to defaults
        glUniform1i(glGetUniformLocation(forwardProgram.handle, "uEnvironmentEnabled"), 0);
        glUniform1i(glGetUniformLocation(forwardProgram.handle, "uNormalMapAvailable"), 0);
        glUniform1f(glGetUniformLocation(forwardProgram.handle, "uHeightScale"), 0.0f);
        glUniform1f(glGetUniformLocation(forwardProgram.handle, "uReflectionIntensity"), 0.0f);

        // Reset ALL texture units before each entity
        for (int i = 0; i < 4; i++) {
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, 0);
            glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
        }

        // Render all entities (except skybox)
        for (auto& entity : app->entities) {
            if (!entity.active || entity.name == "SkyBox") continue;

            Model& model = app->models[entity.modelIndex];
            Mesh& mesh = app->meshes[model.meshIdx];

            glUniformMatrix4fv(glGetUniformLocation(forwardProgram.handle, "uModel"), 1, GL_FALSE, glm::value_ptr(entity.worldMatrix));

            for (size_t i = 0; i < mesh.submeshes.size(); ++i) {
                if (i >= model.materialIdx.size()) continue;

                // Reset all texture units
                for (int texUnit = 0; texUnit < 4; ++texUnit) {
                    glActiveTexture(GL_TEXTURE0 + texUnit);
                    glBindTexture(GL_TEXTURE_2D, 0);
                    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
                }

                u32 matIdx = model.materialIdx[i];
                Material& mat = app->materials[matIdx];

                // Albedo
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, app->textures[mat.albedoTextureIdx].handle);
                glUniform1i(glGetUniformLocation(forwardProgram.handle, "uAlbedoTexture"), 0);

                // Normal map
                if (mat.normalsTextureIdx != 0) {
                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, app->textures[mat.normalsTextureIdx].handle);
                    glUniform1i(glGetUniformLocation(forwardProgram.handle, "uNormalMap"), 1);
                    glUniform1i(glGetUniformLocation(forwardProgram.handle, "uNormalMapAvailable"), 1);
                } else {
                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, 0);
                    glUniform1i(glGetUniformLocation(forwardProgram.handle, "uNormalMap"), 1);
                    glUniform1i(glGetUniformLocation(forwardProgram.handle, "uNormalMapAvailable"), 0);
                }

                // Height map
                if (mat.heighTextureIdx != 0) {
                    glActiveTexture(GL_TEXTURE2);
                    glBindTexture(GL_TEXTURE_2D, app->textures[mat.heighTextureIdx].handle);
                    glUniform1i(glGetUniformLocation(forwardProgram.handle, "uHeightMap"), 2);
                    glUniform1f(glGetUniformLocation(forwardProgram.handle, "uHeightScale"), app->reliefIntensity);
                } else {
                    glActiveTexture(GL_TEXTURE2);
                    glBindTexture(GL_TEXTURE_2D, 0);
                    glUniform1i(glGetUniformLocation(forwardProgram.handle, "uHeightMap"), 2);
                    glUniform1f(glGetUniformLocation(forwardProgram.handle, "uHeightScale"), 0.0f);
                }

                // --- Reflection/environment map: only for Enviroment_Map entities --
                if (entity.type == EntityType::Enviroment_Map) {
                    glActiveTexture(GL_TEXTURE3);
                    glBindTexture(GL_TEXTURE_CUBE_MAP, app->cubeMap.cubeMapTexture);
                    glUniform1i(glGetUniformLocation(forwardProgram.handle, "uSkybox"), 3);
                    glUniform1i(glGetUniformLocation(forwardProgram.handle, "uEnvironmentEnabled"), 1);;
 
                    glUniform1f(glGetUniformLocation(forwardProgram.handle, "diffus_amb"), app->diffuse);
                } else {
                    glActiveTexture(GL_TEXTURE3);
                    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
                    glUniform1i(glGetUniformLocation(forwardProgram.handle, "uSkybox"), 3);
                    glUniform1i(glGetUniformLocation(forwardProgram.handle, "uEnvironmentEnabled"), 0);
                    glUniform1f(glGetUniformLocation(forwardProgram.handle, "diffus_amb"), app->diffuse);
                }

                // Draw submesh
                glBindVertexArray(FindVao(mesh, i, forwardProgram));
                glDrawElements(GL_TRIANGLES, mesh.submeshes[i].indices.size(), GL_UNSIGNED_INT, (void*)(uintptr_t)mesh.submeshes[i].indexOffset);
                glBindVertexArray(0);
            }
        }
        break;
    }
        case Mode_Deferred_Geometry:
        {
            glBindFramebuffer(GL_FRAMEBUFFER, app->primaryFBO.handle);

            // Set up draw buffers
            std::vector<GLuint> textures;
            for (auto& it : app->primaryFBO.attachments)
            {
                textures.push_back(it.second);
            }
            glDrawBuffers(textures.size(), textures.data());

            // Clear buffers
            glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glViewport(0, 0, app->displaySize.x, app->displaySize.y);

            for (const auto& entity : app->entities)
            {
                if (entity.active == false)
                {
                    continue;
                }
                // Determinar which shader se usa
                Program* program = &app->programs[app->geometryProgramIdx];
                if (app->programs[app->reliefMappingIdx].programName == "RELIEF_MAPPING" &&
                    entity.type == EntityType::Relief_Mapping)
                {
                    program = &app->programs[app->reliefMappingIdx];
                }
                
                if (app->programs[app->environmentMapIdx].programName == "REFLECTION_ENVIRONMENT" &&
                    entity.type == EntityType::Enviroment_Map)
                {
                    program = &app->programs[app->environmentMapIdx];

                    glActiveTexture(GL_TEXTURE3);
                    glBindTexture(GL_TEXTURE_CUBE_MAP, app->cubeMap.cubeMapTexture);
                   
                }
                glUseProgram(program->handle);

                glUniformMatrix4fv(glGetUniformLocation(program->handle, "uModel"), 1, GL_FALSE, glm::value_ptr(entity.worldMatrix));
                glUniformMatrix4fv(glGetUniformLocation(program->handle, "uView"), 1, GL_FALSE, glm::value_ptr(app->worldCamera.viewMatrix));
                glUniformMatrix4fv(glGetUniformLocation(program->handle, "uProj"), 1, GL_FALSE, glm::value_ptr(app->worldCamera.projectionMatrix));
                glUniform3fv(glGetUniformLocation(program->handle, "uCameraPosition"), 1, glm::value_ptr(app->worldCamera.position));

                glBindBufferRange(GL_UNIFORM_BUFFER, 0, app->globalUBO.handle, 0, app->globalUBO.size);
                glBindBufferRange(GL_UNIFORM_BUFFER, 1, app->entityUBO.handle, entity.entityBufferOffset, entity.entityBufferSize);

                Model& model = app->models[entity.modelIndex];
                Mesh& mesh = app->meshes[model.meshIdx];

                for (size_t i = 0; i < mesh.submeshes.size(); ++i)
                {
                    GLuint vao = FindVao(mesh, i, *program);
                    glBindVertexArray(vao);

                    u32 matIdx = model.materialIdx[i];
                    Material& mat = app->materials[matIdx];


                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, app->textures[mat.albedoTextureIdx].handle);
                    glUniform1i(glGetUniformLocation(program->handle, "uDiffuse"), 0);

                    if (mat.normalsTextureIdx != 0)
                    {
                        glActiveTexture(GL_TEXTURE1);
                        glBindTexture(GL_TEXTURE_2D, app->textures[mat.normalsTextureIdx].handle);
                        glUniform1i(glGetUniformLocation(program->handle, "uNormalMap"), 1);
                    }

                    if (program == &app->programs[app->reliefMappingIdx])
                    {
                        glActiveTexture(GL_TEXTURE2);
                        glBindTexture(GL_TEXTURE_2D, app->textures[mat.heighTextureIdx].handle);
                        glUniform1i(glGetUniformLocation(program->handle, "uHeightMap"), 2);

                        glUniform3fv(glGetUniformLocation(program->handle, "uViewPos"), 1, glm::value_ptr(app->worldCamera.position));
                        glUniform1f(glGetUniformLocation(program->handle, "uHeightScale"), app->reliefIntensity );
                        glUniform1i(glGetUniformLocation(program->handle, "uViewMode"), app->reliefViewMode);


                    }
                    if (program == &app->programs[app->environmentMapIdx])
                    {
                        glUniform1i(glGetUniformLocation(program->handle, "skybox"), 3);

                        glUniform1i(glGetUniformLocation(program->handle, "cubeMapType"), app->cubemapView);
                        glUniform1i(glGetUniformLocation(program->handle, "uDebugType"), 1);
                       
                        glUniform1f(glGetUniformLocation(program->handle, "diffus_amb"), app->diffuse);
                    }
                   
                    Submesh& submesh = mesh.submeshes[i];
                    glDrawElements(GL_TRIANGLES, submesh.indices.size(), GL_UNSIGNED_INT, (void*)(uintptr_t)submesh.indexOffset);

                    glBindTexture(GL_TEXTURE_2D, 0);
                }

                glBindVertexArray(0);
                glUseProgram(0);
            }
            if (app->pgaType == 3)
            {
                RenderCubeMap(app);
                
            }
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glUseProgram(0);
            if (app->pgaType == 1 || 2)
            {
                RenderScreenFillQuad(app, app->primaryFBO);
            }
            
        }
        break;

    default:;
    }
}

void CleanUp(App* app)
{
    ELOG("Cleaning up engine");

    for (auto& texture : app->textures)
    {
        glDeleteTextures(1, &texture.handle);
    };

    for (auto& program : app->programs)
    {
        program.handle = 0;
    };


    if (app->vao != 0)
    {
        glDeleteVertexArrays(1, &app->vao);
        app->vao = 0;
    }
    if (app->embeddedVertices.handle != 0)
    {
        glDeleteBuffers(1, &app->embeddedVertices.handle);
        app->embeddedVertices.handle = 0;
    }
    if (app->embeddedElements != 0)
    {
        glDeleteBuffers(1, &app->embeddedElements);
        app->embeddedElements = 0;
    }

    for (auto data : app->cubeMap.faces1Data) {
        if (data) stbi_image_free(data);
    }
    for (auto data : app->cubeMap.faces2Data) {
        if (data) stbi_image_free(data);
    }
    if (app->cubeMap.VAO != 0)
    {
        glDeleteVertexArrays(1, &app->cubeMap.VAO);
        app->cubeMap.VAO = 0;
    }
    app->primaryFBO.Clear();
}

GLuint FindVao(Mesh& mesh, u32 submeshIndex, const Program& program)
{
    Submesh& submesh = mesh.submeshes[submeshIndex];

    for (u32 i = 0; i < submesh.vaos.size(); ++i) {
        if (submesh.vaos[i].programHandle == program.handle) {
            return submesh.vaos[i].handle;
        }
    }

    GLuint vaoHandle;

    glGenVertexArrays(1, &vaoHandle);
    glBindVertexArray(vaoHandle);

    glBindBuffer(GL_ARRAY_BUFFER, mesh.vertexBufferHandle);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.indexBufferHandle);

    for (auto& shaderLayout : program.vertexInputLayout.attributes)
    {
        bool attributeWasLinked = false;

        for (auto& meshLayout : submesh.vertexBufferLayout.attributes)
        {
            if (shaderLayout.location == meshLayout.location)
            {
                const u32 index = meshLayout.location;
                const u32 ncomp = meshLayout.componentCount;
                const u32 offset = meshLayout.offset + submesh.vertexOffset;
                const u32 stride = submesh.vertexBufferLayout.stride;
                               glVertexAttribPointer(index, ncomp, GL_FLOAT, GL_FALSE, stride, (void*)(u64)offset);
                glEnableVertexAttribArray(index);

                attributeWasLinked = true;
                break;
            }
        }
        assert(attributeWasLinked);
    }

    glBindVertexArray(0);

    Vao vao = { vaoHandle, program.handle };
    submesh.vaos.push_back(vao);

    return vaoHandle;
}
