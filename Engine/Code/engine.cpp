﻿//
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


void CreateEntity(App* app, const u32 aModelIndx, const glm::mat4& aVP, const glm::mat4& aPosition)
{
   
    Entity entity;
    AlignHead(app->entityUBO, app->uniformBlockAlignment);
    entity.entityBufferOffset = app->entityUBO.head;

    entity.worldMatrix = aPosition;
    entity.modelIndex = aModelIndx;

    PushMat4(app->entityUBO, entity.worldMatrix);
    PushMat4(app->entityUBO, aVP * entity.worldMatrix);

    entity.entityBufferSize = app->entityUBO.size - entity.entityBufferOffset;

    app->entities.push_back(entity);
}
void CreateLight(App* app, LightType light, vec3 color, vec3 position, float intensity)
{
    glm::mat4 VP = app->worldCamera.projectionMatrix * app->worldCamera.viewMatrix;

    if (light == LightType::Light_Directional)
    {
        app->lights.push_back({ light, color, vec3(1,0,0), position, intensity });
    }
    else
    {
        vec3 spherePosition = position;
        glm::mat4 sphereWorld = glm::translate(spherePosition);

        CreateEntity(app, app->sphereIdx, VP, sphereWorld);

        app->lights.push_back({ light, color, vec3(0), position, intensity, static_cast<int>(app->entities.size() - 1) });
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
        (GLint) strlen(versionString),
        (GLint) strlen(shaderNameDefine),
        (GLint) strlen(vertexShaderDefine),
        (GLint) programSource.len
    };
    const GLchar* fragmentShaderSource[] = {
        versionString,
        shaderNameDefine,
        fragmentShaderDefine,
        programSource.str
    };
    const GLint fragmentShaderLengths[] = {
        (GLint) strlen(versionString),
        (GLint) strlen(shaderNameDefine),
        (GLint) strlen(fragmentShaderDefine),
        (GLint) programSource.len
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


GLuint CreateTexture2DFromImage(Image image)
{
    GLenum internalFormat = GL_RGB8;
    GLenum dataFormat     = GL_RGB;
    GLenum dataType       = GL_UNSIGNED_BYTE;

    switch (image.nchannels)
    {
        case 3: dataFormat = GL_RGB; internalFormat = GL_RGB8; break;
        case 4: dataFormat = GL_RGBA; internalFormat = GL_RGBA8; break;
        default: ELOG("LoadTexture2D() - Unsupported number of channels");
    }

    GLuint texHandle;
    glGenTextures(1, &texHandle);
    glBindTexture(GL_TEXTURE_2D, texHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, image.size.x, image.size.y, 0, dataFormat, dataType, image.pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    return texHandle;
}
void FreeImage(Image image)
{
    stbi_image_free(image.pixels);
}

u32 LoadTexture2D(App* app, const char* filepath)
{
    for (u32 texIdx = 0; texIdx < app->textures.size(); ++texIdx)
        if (app->textures[texIdx].filepath == filepath)
            return texIdx;

    Image image = LoadImage(filepath);

    if (image.pixels)
    {
        Texture tex = {};
        tex.handle = CreateTexture2DFromImage(image);
        tex.filepath = filepath;

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
    glUniform1f(glGetUniformLocation(program.handle, "uDepthIntensity"), app->depthIntensity);

    // Render quad
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);

    glBindVertexArray(0);
    glUseProgram(0);
    for (int i = 0; i < 5; ++i) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    glActiveTexture(GL_TEXTURE0);
}
void SetUpCamera(App* app)
{
    app->worldCamera.position = glm::vec3(10, 15, 50);
    app->worldCamera.worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
    app->worldCamera.yaw = -90.0f;
    app->worldCamera.pitch = 0.0f;
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

void Init(App* app)
{
    ExtensionsOpenGL(app);

    SetUpCamera(app);

    InitMeshBuffers(app);

    app->texturedGeometryProgramIdx = LoadProgram(app, "Render_Quad.glsl", "Render_Quad");

    app->programUniformTexture = glGetUniformLocation(app->programs[app->texturedGeometryProgramIdx].handle, "uTexture");

    app->whiteTexIdx = LoadTexture2D(app, "color_white.png");

    app->patrickIdx = LoadModel(app, "Thun/Thun.obj");
    
    u32 planeIdx = LoadModel(app, "./Plane.obj");
    
    app->sphereIdx = LoadModel(app, "./Sphere.obj");
   
    u32 test_1 = LoadModel(app, "./Entity_test.obj");
    app->geometryProgramIdx = LoadProgram(app, "RENDER_GEOMETRY.glsl", "RENDER_GEOMETRY");
    app->patrickTextureUniform = glGetUniformLocation(app->programs[app->geometryProgramIdx].handle, "uTexture");

    float aspectRatio = (float)app->displaySize.x / (float)app->displaySize.y;
    float near = 0.1f;
    float far = 1000.0f;
    app->worldCamera.projectionMatrix = glm::perspective(glm::radians(60.0f), aspectRatio, near, far);
    app->worldCamera.viewMatrix = glm::lookAt(vec3(10,15, 50), vec3(0, 1, 0), vec3(0, 1, 0));
    

    glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &app->maxUniformBufferSize);
    glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &app->uniformBlockAlignment);

    app->globalUBO = CreateConstantBuffer(app->maxUniformBufferSize);
    app->entityUBO = CreateConstantBuffer(app->maxUniformBufferSize);
    
    CreateLight(app, LightType::Light_Directional, vec3(1),vec3(0), 1.f);
    
    UpdateLights(app);

    Buffer& entityUBO = app->entityUBO;
    MapBuffer(entityUBO, GL_WRITE_ONLY);
    glm::mat4 VP = app->worldCamera.projectionMatrix * app->worldCamera.viewMatrix;

    CreateEntity(app, test_1, VP, glm::translate(glm::vec3(0, 0, 0)));
    
    CreateEntity(app, planeIdx, VP, glm::identity<glm::mat4>());

    CreateEntity(app, app->patrickIdx, VP, glm::translate(glm::vec3(6, 0, 5)));
    CreateEntity(app, app->patrickIdx, VP, glm::translate(glm::vec3(0, 0, 0)));

    UnmapBuffer(entityUBO);

    app->mode = Mode_Forward_Geometry;

    app->primaryFBO.CreateFBO(4, app->displaySize.x, app->displaySize.y);
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

        //Todo: Poner ventana aparte
        //ImGui::Image((ImTextureID)(intptr_t)textureID, viewportSize, ImVec2(0, 1), ImVec2(1, 0));


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

    ImGui::End();
    ImGui::Begin("Inspector");
    {
        ImGui::Text("FPS: %.1f", 1.0f / app->deltaTime);
        ImGui::Separator();

        if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::DragFloat3("Position", &app->worldCamera.position[0], 0.1f);
            ImGui::DragFloat("Speed", &app->worldCamera.movementSpeed, 0.1f, 1.0f, 100.0f);
            ImGui::DragFloat("Sensitivity", &app->worldCamera.mouseSensitivity, 0.01f, 0.01f, 1.0f);
        }

        if (ImGui::CollapsingHeader("Lights", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImGui::Button("Add Directional Light")) {
                CreateLight(app, LightType::Light_Directional, vec3(1), vec3(0), 1.f);
                UpdateLights(app);
            }
            ImGui::SameLine();
            if (ImGui::Button("Add Point Light")) {
                CreateLight(app, LightType::Light_Point, vec3(1), vec3(0), 1.f);
                UpdateLights(app);
            }
            ImGui::SameLine();
            if (ImGui::Button("Add 2000 Point Lights"))
            {
                for (size_t i = 0; i < 1000; i++)
                {
                    CreateLight(app, LightType::Light_Point, vec3(1), vec3(0), 1.f);

                }
                for (size_t i = 0; i < 1000; i++)
                {
                    CreateLight(app, LightType::Light_Point, vec3(1), vec3(0), 1.f);

                }
                UpdateLights(app);
            }
            for (size_t i = 0; i < app->lights.size(); ++i)
            {
                ImGui::PushID(static_cast<int>(i));
                Light& light = app->lights[i];
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
                if (ImGui::SliderFloat("Intensity", &intensity, 0.0f, 10.0f, "%.001f"))
                {
                    light.intensity = intensity;
                    lightChanged = true;
                }

                ImGui::SameLine();
                if (ImGui::Button("Delete"))
                {
                    // Eliminar la entidad asociada si es una luz puntual
                    if (light.type == LightType::Light_Point && light.entityIndex != -1)
                    {
                        app->entities.erase(app->entities.begin() + light.entityIndex);
                        // Ajustar índices de las luces posteriores
                        for (auto& otherLight : app->lights)
                        {
                            if (otherLight.entityIndex > light.entityIndex)
                                otherLight.entityIndex--;
                        }
                    }
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
        for (const auto& ext: app->glExtensions)
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
    for (auto& entity : app->entities) {
        size_t matrixOffset = entity.entityBufferOffset + sizeof(glm::mat4);
        glm::mat4 newVPMatrix = VP * entity.worldMatrix;
        memcpy((char*)app->entityUBO.data + matrixOffset, &newVPMatrix, sizeof(glm::mat4));
    }
    UnmapBuffer(app->entityUBO);
}

void UpdateLights(App* app)
{
    MapBuffer(app->globalUBO, GL_WRITE_ONLY);
    PushMat3(app->globalUBO, app->worldCamera.position);
    PushUInt(app->globalUBO, app->lights.size());

    for (size_t i = 0; i < app->lights.size(); ++i)
    {
        AlignHead(app->globalUBO, sizeof(vec4));
        Light& light = app->lights[i];
        if (light.type == LightType::Light_Point && light.entityIndex != -1)
        {
            if (light.entityIndex < app->entities.size())
            {
                Entity& sphereEntity = app->entities[light.entityIndex];
                glm::vec3 modelOffset = vec3(2.f, .0f, 2.f); 
                glm::mat4 correction = glm::translate(glm::mat4(1.0f), modelOffset);
                sphereEntity.worldMatrix = glm::translate(glm::mat4(1.0f), light.position) * correction;
            }
        }
        PushUInt(app->globalUBO, static_cast<int>(light.type));
        PushVec3(app->globalUBO, light.color * light.intensity);
        PushVec3(app->globalUBO, light.direction);
        PushVec3(app->globalUBO, light.position);
    }

    UnmapBuffer(app->globalUBO);
}

void Render(App* app)
{
    switch (app->mode)
    {
        case Mode_TexturedQuad:
        {
            
        }break;
        case Mode_Forward_Geometry:
        {
            glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glBindFramebuffer(GL_FRAMEBUFFER, app->primaryFBO.handle);

            std::vector<GLuint> textures;
            for (auto& it : app->primaryFBO.attachments)
            {
                textures.push_back(it.second);
            }
            glDrawBuffers(textures.size(), textures.data());

            glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glViewport(0, 0, app->displaySize.x, app->displaySize.y);

            Program& geometryProgram = app->programs[app->geometryProgramIdx];
            glUseProgram(geometryProgram.handle);

            glBindBufferRange(GL_UNIFORM_BUFFER, 0, app->globalUBO.handle, 0, app->globalUBO.size);

            for (const auto& entity : app->entities)
            {
                glBindBufferRange(GL_UNIFORM_BUFFER, 1, app->entityUBO.handle, entity.entityBufferOffset, entity.entityBufferSize);

                Model& model = app->models[entity.modelIndex];
                Mesh& mesh = app->meshes[model.meshIdx];

                for (size_t i = 0; i < mesh.submeshes.size(); ++i)
                {
                    GLuint vao = FindVao(mesh, i, geometryProgram);
                    glBindVertexArray(vao);

                    u32 submeshMaterialIdx = model.materialIdx[i];
                    Material& submeshMaterial = app->materials[submeshMaterialIdx];

                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, app->textures[submeshMaterial.albedoTextureIdx].handle);

                    glUniform1i(app->patrickTextureUniform, 0);

                    Submesh& submesh = mesh.submeshes[i];
                    glDrawElements(GL_TRIANGLES, submesh.indices.size(), GL_UNSIGNED_INT, (void*)(u64)submesh.indexOffset);

                    glBindTexture(GL_TEXTURE_2D, 0);
                }
            }

            glBindFramebuffer(GL_FRAMEBUFFER,0);
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
