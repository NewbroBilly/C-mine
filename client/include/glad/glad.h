#pragma once

// vitaGL provides the OpenGL ES/OpenGL compatibility layer directly. The
// desktop build keeps using the generated glad header from CMake.
#ifdef __VITA__
#include <vitaGL.h>
#ifndef GL_TEXTURE_WRAP_R
#define GL_TEXTURE_WRAP_R 0x8072
#endif
#ifndef GL_TEXTURE_BASE_LEVEL
#define GL_TEXTURE_BASE_LEVEL 0x813C
#endif
#ifndef GL_TEXTURE_MAX_LEVEL
#define GL_TEXTURE_MAX_LEVEL 0x813D
#endif
using GLADloadproc = void* (*)(const char*);
inline int gladLoadGLLoader(GLADloadproc) { return 1; }
#else
#include_next <glad/glad.h>
#endif
