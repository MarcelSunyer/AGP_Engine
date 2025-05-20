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
        app->lights.push_back({ light, color, vec3(1,0,0), position, intensity , mode});
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
void SetUpCamera(App* app)
{
    app->worldCamera.position = glm::vec3(10, 90, 120);
    app->worldCamera.worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
    app->worldCamera.yaw = -90.0f;
    app->worldCamera.pitch = -30.0f;
    app->worldCamera.movementSpeed = 40.0f;
    app->worldCamera.mouseSensitivity = 0.1f;
    app->worldCamera.isRotating = false;
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

void Init(App* app)
{
    app->pgaType = 2;

    ExtensionsOpenGL(app);

    SetUpCamera(app);

    InitMeshBuffers(app);

    app->texturedGeometryProgramIdx = LoadProgram(app, "Render_Quad.glsl", "Render_Quad");

    app->reliefMappingIdx = LoadProgram(app, "Relief_Mapping.glsl", "RELIEF_MAPPING");

    app->programUniformTexture = glGetUniformLocation(app->programs[app->texturedGeometryProgramIdx].handle, "uTexture");

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

    //Comentat ja que no me crea mes d'una esfera jiji
    //app->sphereIdx = LoadModel(app, "Sphere/Sphere_Light.obj");

    u32 test_1 = LoadModel(app, "Test/Entity_test.obj");

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
    CreateLight(app, LightType::Light_Directional, vec3(1.0), vec3(1, 0, 0), 5, 1);
    CreateLight(app, LightType::Light_Directional, vec3(1.0), vec3(1, 0, 0), 5, 2);
    CreateLight(app, LightType::Light_Directional, vec3(1.0), vec3(1, 0, 0), 5, 3);

    Buffer& entityUBO = app->entityUBO;
    MapBuffer(entityUBO, GL_WRITE_ONLY);
    glm::mat4 VP = app->worldCamera.projectionMatrix * app->worldCamera.viewMatrix;

    CreateEntity(app, cube, VP, glm::translate(glm::vec3(30, 0, 0)), "Cube", EntityType::Relief_Mapping);
    
    CreateEntity(app, cube2, VP, glm::translate(glm::vec3(-20, 0, 0)), "Cube2", EntityType::Relief_Mapping);

    CreateEntity(app, cube3, VP, glm::translate(glm::vec3(-70, 0, 0)), "Cube3", EntityType::Relief_Mapping);

    CreateEntity(app, cone, VP, glm::translate(glm::vec3(-30, 0, 0)), "Cone", EntityType::Deferred_Rendering);

    CreateEntity(app, torus, VP, glm::translate(glm::vec3(-30, 0, -30)), "Torus", EntityType::Deferred_Rendering);

    CreateEntity(app, sphere, VP, glm::translate(glm::vec3(30, 0, -30)), "Sphere", EntityType::Deferred_Rendering);

    CreateEntity(app, monkey, VP, glm::translate(glm::vec3(0, 0, 0)), "Monkey", EntityType::Deferred_Rendering);

    CreateEntity(app, planeIdx, VP, glm::identity<glm::mat4>(), "Plane", EntityType::Deferred_Rendering);

    CreateEntity(app, skyBox, VP, glm::identity<glm::mat4>(), "SkyBox", EntityType::Deferred_Rendering);

    CreateEntity(app, app->pikachu, VP, glm::translate(glm::vec3(0, 0, 0)), "Pikachu", EntityType::Deferred_Rendering);

    UnmapBuffer(entityUBO);

    app->mode = Mode_Deferred_Geometry;

    app->primaryFBO.CreateFBO(4, app->displaySize.x, app->displaySize.y);
    UpdateLights(app);
}
void Gui(App* app)
{
    ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New Scene")) {}
            if (ImGui::MenuItem("Save Scene")) {}
            if (ImGui::MenuItem("Exit")) { app->isRunning = false; }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
    ImGui::Begin("Delivery");
    {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));

        const char* viewLabels[] = { "Opción 1", "Opción 2", "Opción 3" };

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
            bool isActive = (static_cast<int>(app->pgaType) == i + 1); // Comienza desde 1

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
            if (app->bufferViewMode == App::BUFFER_VIEW_ALBEDO)
            {
                textureID = app->primaryFBO.attachments[0].second;
            }
            else if (app->bufferViewMode == App::BUFFER_VIEW_NORMALS)
            {
                textureID = app->primaryFBO.attachments[1].second;
            }
            else if (app->bufferViewMode == App::BUFFER_VIEW_POSITION)
            {
                textureID = app->primaryFBO.attachments[2].second;
            }
            else if (app->bufferViewMode == App::BUFFER_VIEW_VIEWDIR)
            {
                textureID = app->primaryFBO.attachments[3].second;
            }
            else if (app->bufferViewMode == App::BUFFER_VIEW_DEPTH)
            {
                textureID = app->primaryFBO.depthHandle;
            }

            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));

            const char* viewLabels[] = { "Main", "Albedo", "Normals", "Position", "ViewDir" ,"Depth" };
            {
                ImGuiStyle& style = ImGui::GetStyle();

                float size = ImGui::CalcTextSize(viewLabels[0]).x + style.FramePadding.x * 2.0f;
                float avail = ImGui::GetContentRegionAvail().x;

                float off = (avail - size) * 0.4;

                if (off > 0.0f)
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + off);
            }
            for (int i = 0; i <= 5; i++) {
                bool isActive = (static_cast<int>(app->bufferViewMode) == i);
                if (isActive) {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.4f, 0.8f, 1.0f));
                }


                if (ImGui::Button(viewLabels[i], ImVec2(80, 30))) {
                    app->bufferViewMode = static_cast<App::BufferViewMode>(i);
                    if (i != 0)
                    {
                        app->showDepthOverlay = false;
                    }
                }
                if (isActive) {
                    ImGui::PopStyleColor();
                }
                if (i < 5)
                {
                    ImGui::SameLine();
                }
            }
            ImGui::PopStyleVar();
        }
        ImGui::End();
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

                if (app->entities[i].modelIndex == app->pikachu || app->entities[i].name == " " || app->entities[i].name == "SkyBox") {
                    //No registra las entities
                }
                else {
                    if (app->entities[i].type == EntityType::Deferred_Rendering && app->pgaType == 0)
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


        if (ImGui::CollapsingHeader("Important Info", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Text("FPS: %.1f", 1.0f / app->deltaTime);
           
            if (ImGui::CollapsingHeader("Relief Mapping") || app->pgaType ==2)
            {
                ImGui::SliderFloat("Relief Intensity", &app->reliefIntensity, 0.0f, 0.2f, "%.3f");
            }
        }
        ImGui::Separator();

        if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::DragFloat3("Position", &app->worldCamera.position[0], 0.1f);
            ImGui::DragFloat("Speed", &app->worldCamera.movementSpeed, 0.1f, 1.0f, 100.0f);
            ImGui::DragFloat("Sensitivity", &app->worldCamera.mouseSensitivity, 0.01f, 0.01f, 1.0f);
        }

        if (ImGui::CollapsingHeader("Lights", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImGui::Button("Add Directional Light")) {
                CreateLight(app, LightType::Light_Directional, vec3(1), vec3(0), 1.f, app->pgaType);
                UpdateLights(app);
            }
            ImGui::SameLine();
            if (ImGui::Button("Add Point Light")) {
                CreateLight(app, LightType::Light_Point, vec3(1), vec3(0), 1.f, app->pgaType);
                UpdateLights(app);
            }

            if (ImGui::Button("Add 400 Point Lights"))
            {
                for (int x = -10; x < 10; ++x)
                {
                    for (int z = -10; z < 10; ++z)
                    {
                        glm::vec3 color = glm::vec3(rand() % 100 / 100.0f, rand() % 100 / 100.0f, rand() % 100 / 100.0f);
                        CreateLight(app, LightType::Light_Point, color, glm::vec3(x * 50, 0.0f, z * 50), 10, app->pgaType);
                    }
                }
                UpdateLights(app);
            }

            ImGui::SameLine();
            if (ImGui::Button("Delete Last 400 lights"))
            {
                if (!app->lights.empty()) {

                    size_t lightsToRemove = glm::min(app->lights.size(), (size_t)401);

                    app->lights.erase(app->lights.end() - lightsToRemove, app->lights.end());

                    UpdateLights(app);
                }
            }

            u32 sceneLights;
            for (size_t i = 0; i < app->lights.size(); i++)
            {
                sceneLights = i;
            }

            ImGui::Text("Lights in scene:  %d", sceneLights);
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

                ImGui::SameLine();
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
        }
        ImGui::Separator();

        ImGui::Begin("OpenGl Info");
        ImGui::Text("Extensions");
        for (const auto& ext : app->glExtensions)
        {
            ImGui::Text("%s", ext.c_str());
        }
        ImGui::End();

    }
}

void UpdateCameraVectors(Camera* camera) {

    glm::vec3 front;
    front.x = cos(glm::radians(camera->yaw)) * cos(glm::radians(camera->pitch));
    front.y = sin(glm::radians(camera->pitch));
    front.z = sin(glm::radians(camera->yaw)) * cos(glm::radians(camera->pitch));
    camera->front = glm::normalize(front);

    camera->right = glm::normalize(glm::cross(camera->front, camera->worldUp));
    camera->up = glm::normalize(glm::cross(camera->right, camera->front));
}

void ProcessMouseMovement(Camera* camera, float xoffset, float yoffset) {

    xoffset *= camera->mouseSensitivity;
    yoffset *= camera->mouseSensitivity;

    camera->yaw += xoffset;
    camera->pitch -= yoffset;

    if (camera->pitch > 89.0f) camera->pitch = 89.0f;
    if (camera->pitch < -89.0f) camera->pitch = -89.0f;

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
    if (app->pgaType == 1 && !entity->type == EntityType::Deferred_Rendering)
    {
        entity->active = false;
    }

    if (app->pgaType == 2 && !entity->type == EntityType::Relief_Mapping)
    {
        entity->active = false;
    }

    if (app->pgaType == 3 && !entity->type == EntityType::Enviroment_Map)
    {
        entity->active = false;
    }
}
void Update(App* app) {
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
            // Animación flotante de Pikachu
            float yPos = sin(animationTime) * 7.5f;
            glm::mat4 translation = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, yPos, -30.0f));
            entity.worldMatrix = translation;
        }

        ActiveEntities(app, &entity);

        DisableEntities(app, &entity);

        glm::mat4 vpMatrix = VP * entity.worldMatrix;
        glm::mat4 normalMatrix = glm::transpose(glm::inverse(entity.worldMatrix));

        memcpy((char*)app->entityUBO.data + entity.entityBufferOffset, &entity.worldMatrix, sizeof(glm::mat4));

        memcpy((char*)app->entityUBO.data + entity.entityBufferOffset + sizeof(glm::mat4), &vpMatrix, sizeof(glm::mat4));

        memcpy((char*)app->entityUBO.data + entity.entityBufferOffset + 2 * sizeof(glm::mat4), &normalMatrix, sizeof(glm::mat4));
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
    BindBuffer(app->globalUBO);

    // Subir posición de cámara y número de luces activas
    PushVec3(app->globalUBO, app->worldCamera.position);
    PushUInt(app->globalUBO, activeLightCount);

    // Subir solo las luces del modo activo
    for (auto& light : app->lights) {
        if (light.mode != app->pgaType) continue;

        AlignHead(app->globalUBO, 16);
        PushUInt(app->globalUBO, static_cast<int>(light.type));
        PushVec3(app->globalUBO, light.color * light.intensity);
        PushVec3(app->globalUBO, light.direction);
        PushVec3(app->globalUBO, light.position);
        AlignHead(app->globalUBO, 16);
    }

    UnmapBuffer(app->globalUBO);
}


void Render(App* app)
{
    switch (app->mode)
    {
    case Mode_Forward_Geometry:
        {

        }break;
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
                    entity.name == "Cube" || entity.name == "Cube2" || entity.name == "Cube3")
                {
                    program = &app->programs[app->reliefMappingIdx];
                }

                glUseProgram(program->handle);

                // Matrices and camera
                glUniformMatrix4fv(glGetUniformLocation(program->handle, "uModel"), 1, GL_FALSE, glm::value_ptr(entity.worldMatrix));
                glUniformMatrix4fv(glGetUniformLocation(program->handle, "uView"), 1, GL_FALSE, glm::value_ptr(app->worldCamera.viewMatrix));
                glUniformMatrix4fv(glGetUniformLocation(program->handle, "uProj"), 1, GL_FALSE, glm::value_ptr(app->worldCamera.projectionMatrix));
                glUniform3fv(glGetUniformLocation(program->handle, "uCameraPosition"), 1, glm::value_ptr(app->worldCamera.position));

                // Bind UBOs
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
                    }

                    Submesh& submesh = mesh.submeshes[i];
                    glDrawElements(GL_TRIANGLES, submesh.indices.size(), GL_UNSIGNED_INT, (void*)(uintptr_t)submesh.indexOffset);

                    glBindTexture(GL_TEXTURE_2D, 0);
                }

                glBindVertexArray(0);
                glUseProgram(0);
            }

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glUseProgram(0);
            RenderScreenFillQuad(app, app->primaryFBO);
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
