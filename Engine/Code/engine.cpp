//
// engine.cpp : Put all your graphics stuff in this file. This is kind of the graphics module.
// In here, you should type all your OpenGL commands, and you can also type code to handle
// input platform events (e.g to move the camera or react to certain shortcuts), writing some
// graphics related GUI options, and so on.
//

#include "engine.h"

#include "../AssimpModelLoading.h"

#include <imgui.h>
#include <stb_image.h>
#include <stb_image_write.h>

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
        GLint AttribuitCount = 0UL;
        glGetProgramiv(program.handle, GL_ACTIVE_ATTRIBUTES, &AttribuitCount);
            for (size_t i = 0; i < AttribuitCount; ++i)
            {
                GLchar Name[248];
                GLsizei realNameSize = 0UL;
                GLsizei attribSize = 0UL;
                GLenum attribType;
                glGetActiveAttrib(program.handle, i, ARRAY_COUNT(Name), &realNameSize, &attribSize, &attribType, Name);
                GLuint attribLocation = glGetAttribLocation(program.handle, Name);
                program.vertexInputLayout.attributes.push_back({static_cast<u8>(attribLocation), static_cast<u8>(attribSize)});
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

void FreeImage(Image image)
{
    stbi_image_free(image.pixels);
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

void Init(App* app)
{

    glEnable(GL_DEPTH_TEST);
    //VBO Init
    glGenBuffers(1, &app->embeddedVertices);
    glBindBuffer(GL_ARRAY_BUFFER, app->embeddedVertices);
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
    glBindBuffer(GL_ARRAY_BUFFER, app->embeddedVertices);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexV3V2), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(VertexV3V2), (void*)12);
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, app->embeddedElements);
    glBindVertexArray(0);

    //Program Init
    app->texturedGeometryProgramIdx = LoadProgram(app, "Render_Quad.glsl", "Render_Quadt");

    Program& quadprogram = app->programs[app->texturedGeometryProgramIdx];


    Program& texturedGeometryProgram = app->programs[app->texturedGeometryProgramIdx];
    app->programUniformTexture = glGetUniformLocation(texturedGeometryProgram.handle, "uTexture");

    //Texture Init
    app->diceTexIdx = LoadTexture2D(app, "dice.png");
    app->whiteTexIdx = LoadTexture2D(app, "color_white.png");
    app->blackTexIdx = LoadTexture2D(app, "color_black.png");
    app->normalTexIdx = LoadTexture2D(app, "color_normal.png");
    app->magentaTexIdx = LoadTexture2D(app, "color_magenta.png");

    //Geometry rendering loads
    app->patrickIdx = LoadModel(app, "./Patrick/Patrick.obj");
    app->geometryProgramIdx = LoadProgram(app, "RENDER_GEOMETRY.glsl", "RENDER_GEOMETRY");
    app->patrickTextureUniform = glGetUniformLocation(app->programs[app->geometryProgramIdx].handle, "uTexture");

    app->mode = Mode_Forward_Geometry;


}

void Gui(App* app)
{
    ImGui::Begin("Info");
    ImGui::Text("FPS: %f", 1.0f/app->deltaTime);

    ImGui::Text("OpenGL Version: %s", glGetString(GL_VERSION));
    ImGui::Text("OpenGL Renderer: %s", glGetString(GL_RENDERER));
    ImGui::Text("OpenGL Vendor: %s", glGetString(GL_VENDOR));
    ImGui::Text("OpenGL GLSL Version: %s", glGetString(GL_SHADING_LANGUAGE_VERSION));

    // Display OpenGL extensions
    ImGui::Separator();
    ImGui::Text("OpenGL Extensions:");
    GLint num_extensions;
    glGetIntegerv(GL_NUM_EXTENSIONS, &num_extensions);
    for (int i = 0; i < num_extensions; ++i) {
        const unsigned char* extension = glGetStringi(GL_EXTENSIONS, GLuint(i));
        ImGui::Text("%s", extension);
    }

    // End the ImGui window
    ImGui::End();
}

void Update(App* app)
{
    // You can handle app->input keyboard/mouse here
}

void Render(App* app)
{
    switch (app->mode)
    {
        case Mode_TexturedQuad:
            {
                // TODO: Draw your textured quad here!
                // - clear the framebuffer
                // - set the viewport
                // - set the blending state
                // - bind the texture into unit 0
                // - bind the program 
                //   (...and make its texture sample from unit 0)
                // - bind the vao
                // - glDrawElements() !!!
            glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // Set the viewport
            glViewport(0, 0, app->displaySize.x, app->displaySize.y);

            //Bind the program
            Program& programTexturedGeometry = app->programs[app->texturedGeometryProgramIdx];
            glUseProgram(programTexturedGeometry.handle);
            //Bind the VAO
            glBindVertexArray(app->vao);

            //Set the blending state
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            //Bind the texture into unit 0 (and make its texture sample from unit 0)
            glUniform1i(app->programUniformTexture, 0);
            glActiveTexture(GL_TEXTURE0);
            GLuint textureHandle = app->textures[app->diceTexIdx].handle;
            glBindTexture(GL_TEXTURE_2D, textureHandle);

            //glDrawElements() -> De momento hardcoded a 6
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);

            glBindVertexArray(0);

            glUseProgram(0);
            }
            break;
        case Mode_Forward_Geometry:
        {
            glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // Set the viewport
            glViewport(0, 0, app->displaySize.x, app->displaySize.y);

            Program& geometryProgram = app->programs[app->texturedGeometryProgramIdx];
            glUseProgram(geometryProgram.handle);

            Model& model = app->models[app->patrickIdx];
            Mesh& mesh = app->meshes[model.meshIdx];

            for (size_t i = 0; i < mesh.submeshes.size(); i++)
            {
                GLuint vao = FindVao(mesh, i , geometryProgram);
                glBindVertexArray(vao);

                u32 submeshMaterialIdx = model.materialIdx[i];
                Material& submeshMaterial = app->materials[submeshMaterialIdx];

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, app->textures[submeshMaterial.albedoTextureIdx].handle);
                glUniform1i(app->patrickTextureUniform, 0);
                Submesh& submesh = mesh.submeshes[i];
                glDrawElements(GL_TRIANGLES, submesh.indices.size(), GL_UNSIGNED_INT, (void*)(u64)submesh.indexOffset);

            }
        }
        break;

        default:;
    }
}

void CleanUp(App* app)
{
    
    std::vector<Texture>  textures;
    for (auto& texture : textures)
    {
        glDeleteTextures(1, &texture.handle);
    }

    std::vector<Program>  programs;

    for (auto& program : programs)
    {
        glDeleteTextures(1, &program.handle);
    }

    if(app->vao != 0)
    {
        glDeleteVertexArrays(1, &app->vao);
        app->vao = 0;
    }

    if (app->embeddedVertices != 0)
    {
        glDeleteBuffers(1, &app->embeddedVertices);
        app->embeddedVertices = 0;
    }

    if (app->embeddedElements != 0)
    {
        glDeleteBuffers(1, &app->embeddedElements);
        app->embeddedElements = 0;
    }
}

GLuint FindVao(Mesh& mesh, u32 submeshIndex, const Program& program)
{
    Submesh& submesh = mesh.submeshes[submeshIndex];
    for (u32 i = 0; i < (u32)submesh.vaos.size(); ++i)
    {
        if (submesh.vaos[i].progranHandle == program.handle)
        {
            return submesh.vaos[i].handle;
        }
    }

    GLuint vaoHandle = 0;

    //Create new vao
    {
        glGenVertexArrays(1, &vaoHandle);
        glBindVertexArray(vaoHandle);

        glBindBuffer(GL_ARRAY_BUFFER, mesh.vertexBufferHandle);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.indexBufferHandle);

        for (u32 i = 0; i < program.vertexInputLayout.attributes.size(); ++i)
        {
            bool attributeWasLinked = false;

            for (u32 j = 0; j < submesh.vertexBufferLayout.attributes.size(); ++j)
            {
                if (program.vertexInputLayout.attributes[i].location == submesh.vertexBufferLayout.attributes[j].location)
                {

                    const u32 index = submesh.vertexBufferLayout.attributes[j].location;
                    const u32 ncomp = submesh.vertexBufferLayout.attributes[j].componentCount;
                    const u32 offset = submesh.vertexBufferLayout.attributes[j].offset + submesh.vertexOffset;
                    const u32 stride = submesh.vertexBufferLayout.stride;


                    glVertexAttribPointer(index, ncomp, GL_FLOAT, GL_FALSE, stride, (void*)(u64)submesh.indexOffset);
                    glEnableVertexAttribArray(index);

                    attributeWasLinked = true;
                    break;



                }
            }
            assert(attributeWasLinked);
        }
        glBindVertexArray(0);
    }
      /*  for (auto& shaderLayout : program.vertexInputLayout.attributes)
        {
            bool attributeWasLinked;

            for (auto& meshLayout : submesh.vertexBufferLayout.attributes)
            {
                if (shaderLayout.location == meshLayout.location);
                {
                    const u32 index = meshLayout.location;
                    const u32 ncomp = meshLayout.componentCount;
                    const u32 offset = meshLayout.offset + submesh.vertexOffset;
                    const u32 stride = submesh.vertexBufferLayout.stride;

                    glVertexAttribPointer(index, ncomp, GL_FLOAT, GL_FALSE, stride, (void*)(u64)submesh.indexOffset);
                    glEnableVertexAttribArray(index);

                    attributeWasLinked = true;
                    break;
                }
            }
            assert(attributeWasLinked);
        }
        glBindVertexArray(0);
    }*/

    Vao vao = { vaoHandle, program.handle };
    submesh.vaos.push_back(vao);

    return vaoHandle;
}

