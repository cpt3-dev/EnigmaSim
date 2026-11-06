// wgl implementation of the hand-written GL loader.
// Requires a current OpenGL context (the platform layer creates a temporary
// one first, upgrades to 3.3 Core, and keeps it current before load() runs).

#include "gl_loader.hpp"

#include <windows.h>

#include <cstdio>
#include <cstring>

namespace enigma::gl {
namespace {

char gLoadError[128] = {0};

using GLGETPROC = PROC(APIENTRY*)(const char*);

void* fetchFunction(const char* name) {
    // wglGetProcAddress works for everything introduced after GL 1.1, which
    // covers every function this loader declares.
    const PROC proc = wglGetProcAddress(name);
    return proc != nullptr ? reinterpret_cast<void*>(proc) : nullptr;
}

struct FunctionEntry {
    const char* name;
    void** slot;
};

const FunctionEntry kFunctions[] = {
    {"glCreateShader", reinterpret_cast<void**>(&glCreateShader)},
    {"glShaderSource", reinterpret_cast<void**>(&glShaderSource)},
    {"glCompileShader", reinterpret_cast<void**>(&glCompileShader)},
    {"glGetShaderiv", reinterpret_cast<void**>(&glGetShaderiv)},
    {"glGetShaderInfoLog", reinterpret_cast<void**>(&glGetShaderInfoLog)},
    {"glDeleteShader", reinterpret_cast<void**>(&glDeleteShader)},
    {"glCreateProgram", reinterpret_cast<void**>(&glCreateProgram)},
    {"glAttachShader", reinterpret_cast<void**>(&glAttachShader)},
    {"glLinkProgram", reinterpret_cast<void**>(&glLinkProgram)},
    {"glGetProgramiv", reinterpret_cast<void**>(&glGetProgramiv)},
    {"glGetProgramInfoLog", reinterpret_cast<void**>(&glGetProgramInfoLog)},
    {"glDeleteProgram", reinterpret_cast<void**>(&glDeleteProgram)},
    {"glUseProgram", reinterpret_cast<void**>(&glUseProgram)},
    {"glGetUniformLocation", reinterpret_cast<void**>(&glGetUniformLocation)},
    {"glUniform1f", reinterpret_cast<void**>(&glUniform1f)},
    {"glUniform3f", reinterpret_cast<void**>(&glUniform3f)},
    {"glUniformMatrix4fv", reinterpret_cast<void**>(&glUniformMatrix4fv)},
    {"glGenVertexArrays", reinterpret_cast<void**>(&glGenVertexArrays)},
    {"glBindVertexArray", reinterpret_cast<void**>(&glBindVertexArray)},
    {"glDeleteVertexArrays", reinterpret_cast<void**>(&glDeleteVertexArrays)},
    {"glGenBuffers", reinterpret_cast<void**>(&glGenBuffers)},
    {"glBindBuffer", reinterpret_cast<void**>(&glBindBuffer)},
    {"glDeleteBuffers", reinterpret_cast<void**>(&glDeleteBuffers)},
    {"glBufferData", reinterpret_cast<void**>(&glBufferData)},
    {"glEnableVertexAttribArray", reinterpret_cast<void**>(&glEnableVertexAttribArray)},
    {"glVertexAttribPointer", reinterpret_cast<void**>(&glVertexAttribPointer)},
    {"glDrawArrays", reinterpret_cast<void**>(&glDrawArrays)},
    {"glDrawElements", reinterpret_cast<void**>(&glDrawElements)},
    {"glDepthMask", reinterpret_cast<void**>(&glDepthMask)},
    {"glBlendFunc", reinterpret_cast<void**>(&glBlendFunc)},
    {"glClearColor", reinterpret_cast<void**>(&glClearColor)},
    {"glClear", reinterpret_cast<void**>(&glClear)},
    {"glViewport", reinterpret_cast<void**>(&glViewport)},
    {"glEnable", reinterpret_cast<void**>(&glEnable)},
    {"glDisable", reinterpret_cast<void**>(&glDisable)},
    {"glGetError", reinterpret_cast<void**>(&glGetError)},
};

}  // namespace

PFNGLCREATESHADERPROC glCreateShader = nullptr;
PFNGLSHADERSOURCEPROC glShaderSource = nullptr;
PFNGLCOMPILESHADERPROC glCompileShader = nullptr;
PFNGLGETSHADERIVPROC glGetShaderiv = nullptr;
PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog = nullptr;
PFNGLDELETESHADERPROC glDeleteShader = nullptr;
PFNGLCREATEPROGRAMPROC glCreateProgram = nullptr;
PFNGLATTACHSHADERPROC glAttachShader = nullptr;
PFNGLLINKPROGRAMPROC glLinkProgram = nullptr;
PFNGLGETPROGRAMIVPROC glGetProgramiv = nullptr;
PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog = nullptr;
PFNGLDELETEPROGRAMPROC glDeleteProgram = nullptr;
PFNGLUSEPROGRAMPROC glUseProgram = nullptr;
PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation = nullptr;
PFNGLUNIFORM1FPROC glUniform1f = nullptr;
PFNGLUNIFORM3FPROC glUniform3f = nullptr;
PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv = nullptr;
PFNGLGENVERTEXARRAYSPROC glGenVertexArrays = nullptr;
PFNGLBINDVERTEXARRAYPROC glBindVertexArray = nullptr;
PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays = nullptr;
PFNGLGENBUFFERSPROC glGenBuffers = nullptr;
PFNGLBINDBUFFERPROC glBindBuffer = nullptr;
PFNGLDELETEBUFFERSPROC glDeleteBuffers = nullptr;
PFNGLBUFFERDATAPROC glBufferData = nullptr;
PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray = nullptr;
PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer = nullptr;
PFNGLDRAWARRAYSPROC glDrawArrays = nullptr;
PFNGLDRAWELEMENTSPROC glDrawElements = nullptr;
PFNGLDEPTHMASKPROC glDepthMask = nullptr;
PFNGLBLENDFUNCPROC glBlendFunc = nullptr;
PFNGLCLEARCOLORPROC glClearColor = nullptr;
PFNGLCLEARPROC glClear = nullptr;
PFNGLVIEWPORTPROC glViewport = nullptr;
PFNGLENABLEPROC glEnable = nullptr;
PFNGLDISABLEPROC glDisable = nullptr;
PFNGLGETERRORPROC glGetError = nullptr;

int load() {
    gLoadError[0] = '\0';
    for (const FunctionEntry& entry : kFunctions) {
        void* fn = fetchFunction(entry.name);
        if (fn == nullptr) {
            std::snprintf(gLoadError, sizeof(gLoadError),
                          "OpenGL function not available: %s", entry.name);
            return 1;
        }
        *entry.slot = fn;
    }
    return 0;
}

const char* loadError() {
    return gLoadError;
}

}  // namespace enigma::gl
