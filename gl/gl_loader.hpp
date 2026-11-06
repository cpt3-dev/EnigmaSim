#pragma once
// Hand-written OpenGL 3.3 Core function loader.
// Only the functions actually used by the project are declared and resolved;
// nothing is fetched from the network and no third-party headers are used.
//
// Usage: after the platform layer has created and made current a 3.3 Core
// context, call gl::load(). It returns 0 on success or a non-zero code and
// sets gl::loadError() to the name of the first missing function.

#include <cstddef>
#include <cstdint>

namespace enigma::gl {

// -- GL types (self-contained, no GL/gl.h) -----------------------------------
using GLenum = unsigned int;
using GLboolean = unsigned char;
using GLbitfield = unsigned int;
using GLint = int;
using GLuint = unsigned int;
using GLsizei = int;
using GLchar = char;
using GLfloat = float;
using GLsizeiptr = std::ptrdiff_t;
using GLintptr = std::intptr_t;

#if defined(_WIN32)
#define ENIGMA_GL_APIENTRY __stdcall
#else
#define ENIGMA_GL_APIENTRY
#endif

// -- Constants actually used by the project ----------------------------------
constexpr GLenum GL_NO_ERROR = 0;
constexpr GLenum GL_FALSE = 0;
constexpr GLboolean GL_TRUE_B = 1;  // GLboolean true
constexpr GLenum GL_TRIANGLES = 0x0004;
constexpr GLenum GL_FLOAT = 0x1406;
constexpr GLenum GL_DEPTH_TEST = 0x0B71;
constexpr GLenum GL_BLEND = 0x0BE2;
constexpr GLenum GL_COLOR_BUFFER_BIT = 0x00004000;
constexpr GLenum GL_DEPTH_BUFFER_BIT = 0x00000100;
constexpr GLenum GL_ARRAY_BUFFER = 0x8892;
constexpr GLenum GL_ELEMENT_ARRAY_BUFFER = 0x8893;
constexpr GLenum GL_STATIC_DRAW = 0x88E4;
constexpr GLenum GL_UNSIGNED_INT = 0x1405;
constexpr GLenum GL_CULL_FACE = 0x0B44;
constexpr GLenum GL_MULTISAMPLE = 0x809D;
constexpr GLenum GL_SRC_ALPHA = 0x0302;
constexpr GLenum GL_ONE_MINUS_SRC_ALPHA = 0x0303;
constexpr GLenum GL_VERTEX_SHADER = 0x8B31;
constexpr GLenum GL_FRAGMENT_SHADER = 0x8B30;
constexpr GLenum GL_COMPILE_STATUS = 0x8B81;
constexpr GLenum GL_LINK_STATUS = 0x8B82;

// -- Function pointer typedefs -----------------------------------------------
using PFNGLCREATESHADERPROC = GLuint(ENIGMA_GL_APIENTRY*)(GLenum);
using PFNGLSHADERSOURCEPROC = void(ENIGMA_GL_APIENTRY*)(GLuint, GLsizei,
                                                        const GLchar* const*,
                                                        const GLint*);
using PFNGLCOMPILESHADERPROC = void(ENIGMA_GL_APIENTRY*)(GLuint);
using PFNGLGETSHADERIVPROC = void(ENIGMA_GL_APIENTRY*)(GLuint, GLenum, GLint*);
using PFNGLGETSHADERINFOLOGPROC = void(ENIGMA_GL_APIENTRY*)(GLuint, GLsizei,
                                                            GLsizei*, GLchar*);
using PFNGLDELETESHADERPROC = void(ENIGMA_GL_APIENTRY*)(GLuint);
using PFNGLCREATEPROGRAMPROC = GLuint(ENIGMA_GL_APIENTRY*)(void);
using PFNGLATTACHSHADERPROC = void(ENIGMA_GL_APIENTRY*)(GLuint, GLuint);
using PFNGLLINKPROGRAMPROC = void(ENIGMA_GL_APIENTRY*)(GLuint);
using PFNGLGETPROGRAMIVPROC = void(ENIGMA_GL_APIENTRY*)(GLuint, GLenum, GLint*);
using PFNGLGETPROGRAMINFOLOGPROC = void(ENIGMA_GL_APIENTRY*)(GLuint, GLsizei,
                                                             GLsizei*, GLchar*);
using PFNGLDELETEPROGRAMPROC = void(ENIGMA_GL_APIENTRY*)(GLuint);
using PFNGLUSEPROGRAMPROC = void(ENIGMA_GL_APIENTRY*)(GLuint);
using PFNGLGETUNIFORMLOCATIONPROC = GLint(ENIGMA_GL_APIENTRY*)(GLuint,
                                                               const GLchar*);
using PFNGLUNIFORM1FPROC = void(ENIGMA_GL_APIENTRY*)(GLint, GLfloat);
using PFNGLUNIFORM3FPROC = void(ENIGMA_GL_APIENTRY*)(GLint, GLfloat, GLfloat,
                                                     GLfloat);
using PFNGLUNIFORMMATRIX4FVPROC = void(ENIGMA_GL_APIENTRY*)(GLint, GLsizei,
                                                            GLboolean,
                                                            const GLfloat*);
using PFNGLGENVERTEXARRAYSPROC = void(ENIGMA_GL_APIENTRY*)(GLsizei, GLuint*);
using PFNGLBINDVERTEXARRAYPROC = void(ENIGMA_GL_APIENTRY*)(GLuint);
using PFNGLDELETEVERTEXARRAYSPROC = void(ENIGMA_GL_APIENTRY*)(GLsizei,
                                                              const GLuint*);
using PFNGLGENBUFFERSPROC = void(ENIGMA_GL_APIENTRY*)(GLsizei, GLuint*);
using PFNGLBINDBUFFERPROC = void(ENIGMA_GL_APIENTRY*)(GLenum, GLuint);
using PFNGLDELETEBUFFERSPROC = void(ENIGMA_GL_APIENTRY*)(GLsizei, const GLuint*);
using PFNGLBUFFERDATAPROC = void(ENIGMA_GL_APIENTRY*)(GLenum, GLsizeiptr,
                                                      const void*, GLenum);
using PFNGLENABLEVERTEXATTRIBARRAYPROC = void(ENIGMA_GL_APIENTRY*)(GLuint);
using PFNGLVERTEXATTRIBPOINTERPROC = void(ENIGMA_GL_APIENTRY*)(GLuint, GLint,
                                                               GLenum, GLboolean,
                                                               GLsizei,
                                                               const void*);
using PFNGLDRAWARRAYSPROC = void(ENIGMA_GL_APIENTRY*)(GLenum, GLint, GLsizei);
using PFNGLDRAWELEMENTSPROC = void(ENIGMA_GL_APIENTRY*)(GLenum, GLsizei,
                                                        GLenum, const void*);
using PFNGLDEPTHMASKPROC = void(ENIGMA_GL_APIENTRY*)(GLboolean);
using PFNGLBLENDFUNCPROC = void(ENIGMA_GL_APIENTRY*)(GLenum, GLenum);
using PFNGLCLEARCOLORPROC = void(ENIGMA_GL_APIENTRY*)(GLfloat, GLfloat, GLfloat,
                                                      GLfloat);
using PFNGLCLEARPROC = void(ENIGMA_GL_APIENTRY*)(GLbitfield);
using PFNGLVIEWPORTPROC = void(ENIGMA_GL_APIENTRY*)(GLint, GLint, GLsizei,
                                                    GLsizei);
using PFNGLENABLEPROC = void(ENIGMA_GL_APIENTRY*)(GLenum);
using PFNGLDISABLEPROC = void(ENIGMA_GL_APIENTRY*)(GLenum);
using PFNGLGETERRORPROC = GLenum(ENIGMA_GL_APIENTRY*)(void);

// -- Resolved function pointers (named like the GL functions) ----------------
extern PFNGLCREATESHADERPROC glCreateShader;
extern PFNGLSHADERSOURCEPROC glShaderSource;
extern PFNGLCOMPILESHADERPROC glCompileShader;
extern PFNGLGETSHADERIVPROC glGetShaderiv;
extern PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
extern PFNGLDELETESHADERPROC glDeleteShader;
extern PFNGLCREATEPROGRAMPROC glCreateProgram;
extern PFNGLATTACHSHADERPROC glAttachShader;
extern PFNGLLINKPROGRAMPROC glLinkProgram;
extern PFNGLGETPROGRAMIVPROC glGetProgramiv;
extern PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;
extern PFNGLDELETEPROGRAMPROC glDeleteProgram;
extern PFNGLUSEPROGRAMPROC glUseProgram;
extern PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
extern PFNGLUNIFORM1FPROC glUniform1f;
extern PFNGLUNIFORM3FPROC glUniform3f;
extern PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv;
extern PFNGLGENVERTEXARRAYSPROC glGenVertexArrays;
extern PFNGLBINDVERTEXARRAYPROC glBindVertexArray;
extern PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays;
extern PFNGLGENBUFFERSPROC glGenBuffers;
extern PFNGLBINDBUFFERPROC glBindBuffer;
extern PFNGLDELETEBUFFERSPROC glDeleteBuffers;
extern PFNGLBUFFERDATAPROC glBufferData;
extern PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
extern PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;
extern PFNGLDRAWARRAYSPROC glDrawArrays;
extern PFNGLDRAWELEMENTSPROC glDrawElements;
extern PFNGLDEPTHMASKPROC glDepthMask;
extern PFNGLBLENDFUNCPROC glBlendFunc;
extern PFNGLCLEARCOLORPROC glClearColor;
extern PFNGLCLEARPROC glClear;
extern PFNGLVIEWPORTPROC glViewport;
extern PFNGLENABLEPROC glEnable;
extern PFNGLDISABLEPROC glDisable;
extern PFNGLGETERRORPROC glGetError;

// Resolves every declared function through the current context.
// Returns 0 on success; otherwise the name of the missing function is
// available via loadError() and no pointer is left null.
int load();
[[nodiscard]] const char* loadError();

}  // namespace enigma::gl
