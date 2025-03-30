//
// engine.h: This file contains the types and functions relative to the engine.
//

#pragma once

#include "platform.h"
#include "../AssimpModelLoading.h"
#include "../BufferManagement.h"
#include <glad/glad.h>
#include "Structs.hpp"

void Init(App* app);

void Gui(App* app);

void Update(App* app);

void UpdateLights(App* app);

void Render(App* app);

void CleanUp(App* app);

GLuint FindVao(Mesh& mesh, u32 submeshIndex, const Program& program);

u32 LoadTexture2D(App* app, const char* filepath);
