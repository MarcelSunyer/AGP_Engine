#ifndef BUFFER_MANAGEMENT_H
#define BUFFER_MANAGEMENT_H

#include "Structs.hpp"
#include <stdint.h>  // For uint32_t (u32)
#include <string.h>  // For memcpy
#include <glad/glad.h>


// Type definitions
typedef uint32_t u32;
typedef uint8_t u8;


// Utility functions
bool IsPowerOf2(u32 value);
u32 Align(u32 value, u32 alignment);

// Buffer creation and management
Buffer CreateBuffer(u32 size, GLenum type, GLenum usage);
void BindBuffer(const Buffer& buffer);
void MapBuffer(Buffer& buffer, GLenum access);
void UnmapBuffer(Buffer& buffer);

// Data alignment and pushing
void AlignHead(Buffer& buffer, u32 alignment);
void PushAlignedData(Buffer& buffer, const void* data, u32 size, u32 alignment);

// Macros for buffer creation
#define CreateConstantBuffer(size) CreateBuffer(size, GL_UNIFORM_BUFFER, GL_STREAM_DRAW)
#define CreateStaticVertexBuffer(size) CreateBuffer(size, GL_ARRAY_BUFFER, GL_STATIC_DRAW)
#define CreateStaticIndexBuffer(size) CreateBuffer(size, GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW)

// Macros for pushing data
#define PushData(buffer, data, size) PushAlignedData(buffer, data, size, 1)
#define PushUInt(buffer, value) { u32 v = value; PushAlignedData(buffer, &v, sizeof(v), 4); }
#define PushVec3(buffer, value) PushAlignedData(buffer, value_ptr(value), sizeof(value), sizeof(vec4))
#define PushVec4(buffer, value) PushAlignedData(buffer, value_ptr(value), sizeof(value), sizeof(vec4))
#define PushMat3(buffer, value) PushAlignedData(buffer, value_ptr(value), sizeof(value), sizeof(vec4))
#define PushMat4(buffer, value) PushAlignedData(buffer, value_ptr(value), sizeof(value), sizeof(vec4))
#define PushFloat(buffer, value) PushAlignedData(buffer, &value, sizeof(float), 4)

#endif // BUFFER_MANAGEMENT_H