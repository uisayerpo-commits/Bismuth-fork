#include "common.hpp"
#include "Geode/cocos/CCDirector.h"
#include "renderer/Buffer.hpp"

#include <algorithm>
#include <cstdio>
#include <cstdint>

using namespace geode::prelude;

std::chrono::steady_clock::time_point modStartTime;

$on_mod(Loaded) {
    modStartTime = std::chrono::high_resolution_clock::now();
}

#ifndef GEODE_ANDROID

static void* loadGLProc(const char* name) {
    #ifdef GEODE_WINDOWS
    void* proc = reinterpret_cast<void*>(wglGetProcAddress(name));
    // Some drivers return 1, 2, 3 or -1 instead of null when the function doesn't exist
    uintptr_t value = reinterpret_cast<uintptr_t>(proc);
    if (value <= 3 || value == static_cast<uintptr_t>(-1))
        return nullptr;
    return proc;
    #else
    return nullptr;
    #endif
}

using ProcGlObjectLabel = void(APIENTRY*)(GLenum, GLuint, GLsizei, const char*);

static ProcGlObjectLabel glObjectLabelProc = nullptr;
static bool glObjectLabelLoaded = false;

void glObjectLabel(GLenum id, GLuint name, GLsizei length, const char* label) {
    if (!glObjectLabelLoaded) {
        glObjectLabelProc = reinterpret_cast<ProcGlObjectLabel>(loadGLProc("glObjectLabel"));
        glObjectLabelLoaded = true;
    }
    if (glObjectLabelProc)
        glObjectLabelProc(id, name, length, label);
}

bgl::DispatchComputeProc           bgl::dispatchCompute           = nullptr;
bgl::MultiDrawElementsIndirectProc bgl::multiDrawElementsIndirect = nullptr;
bgl::MemoryBarrierProc             bgl::memoryBarrier             = nullptr;

static u32 contextGlslVersion = 0;

bool bgl::init() {
    static bool initialized = false;
    if (initialized)
        return contextGlslVersion >= 430;

    const char* versionString = (const char*)glGetString(GL_VERSION);
    if (!versionString) // No current context, try again next time
        return false;

    initialized = true;
    contextGlslVersion = 0;

    int major = 0, minor = 0;
    if (sscanf(versionString, "%d.%d", &major, &minor) != 2) {
        log::error("could not parse OpenGL version '{}', Bismuth will not be used", versionString);
        return false;
    }

    if (major < 4 || (major == 4 && minor < 3)) {
        log::error("OpenGL {} is too old, Bismuth needs OpenGL 4.3 or newer. Falling back to the vanilla renderer", versionString);
        return false;
    }

    dispatchCompute           = reinterpret_cast<DispatchComputeProc>(loadGLProc("glDispatchCompute"));
    multiDrawElementsIndirect = reinterpret_cast<MultiDrawElementsIndirectProc>(loadGLProc("glMultiDrawElementsIndirect"));
    memoryBarrier             = reinterpret_cast<MemoryBarrierProc>(loadGLProc("glMemoryBarrier"));

    if (!dispatchCompute || !multiDrawElementsIndirect || !memoryBarrier) {
        log::error("the graphics driver is missing required OpenGL functions (compute: {}, multi draw indirect: {}, memory barrier: {}). Falling back to the vanilla renderer",
            dispatchCompute != nullptr, multiDrawElementsIndirect != nullptr, memoryBarrier != nullptr);
        return false;
    }

    contextGlslVersion = std::min<u32>(major * 100 + minor * 10, 460);
    return true;
}

u32 bgl::glslVersion() {
    return contextGlslVersion >= 430 ? contextGlslVersion : 460;
}

#else // GEODE_ANDROID

bool bgl::init() {
    static int state = 0; // 0: not checked yet, 1: ok, -1: not supported
    if (state != 0)
        return state > 0;

    const char* versionString = (const char*)glGetString(GL_VERSION);
    if (!versionString) // No current context, try again next time
        return false;

    // "OpenGL ES 3.2 v1.r38p0-..." -> 3, 2
    const char* digits = versionString;
    while (*digits && !(*digits >= '0' && *digits <= '9'))
        digits++;

    int major = 0, minor = 0;
    sscanf(digits, "%d.%d", &major, &minor);

    state = -1;

    if (major < 3 || (major == 3 && minor < 1)) {
        log::error("OpenGL ES {}.{} is too old, Bismuth needs OpenGL ES 3.1 (compute shaders). Falling back to the vanilla renderer ('{}')", major, minor, versionString);
        return false;
    }

    // Everything the renderer calls that OpenGL ES 3.0 doesn't have
    bool complete = es::DispatchCompute && es::MemoryBarrier && es::DrawElementsIndirect &&
                    es::BindBufferBase && es::MapBufferRange && es::UnmapBuffer &&
                    es::VertexAttribIPointer && es::GenVertexArrays && es::BindVertexArray;
    if (!complete) {
        log::error("the graphics driver is missing OpenGL ES functions the renderer needs. Falling back to the vanilla renderer");
        return false;
    }

    log::info("OpenGL ES {}.{} ('{}')", major, minor, versionString);
    state = 1;
    return true;
}

u32 bgl::glslVersion() {
    return 310;
}

#endif // GEODE_ANDROID

static float fullscreenQuad[] = {
    -1.0, -1.0,
    -1.0,  1.0,
     1.0,  1.0,
    -1.0, -1.0,
     1.0,  1.0,
     1.0, -1.0
};

u32 fullscreenQuadVAO = 0;
Buffer* fullscreenQuadVBO = nullptr;

void drawFullscreenQuad() {
    if (!fullscreenQuadVAO) {
        if (!fullscreenQuadVBO)
            fullscreenQuadVBO = Buffer::createStaticDraw("Fullscreen quad buffer", fullscreenQuad, sizeof(fullscreenQuad));
        if (!fullscreenQuadVBO)
            return;

        glGenVertexArrays(1, &fullscreenQuadVAO);
        glBindVertexArray(fullscreenQuadVAO);
        fullscreenQuadVBO->bindAs(GL_ARRAY_BUFFER);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, (void*)0);
        glEnableVertexAttribArray(0);
    }

    glBindVertexArray(fullscreenQuadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

#include <Geode/modify/CCDirector.hpp>
class $modify(CommonCCDirector, CCDirector) {
    void purgeDirector() {
        if (fullscreenQuadVAO)
            glDeleteVertexArrays(1, &fullscreenQuadVAO);
        if (fullscreenQuadVBO)
            Buffer::destroy(fullscreenQuadVBO);
        fullscreenQuadVAO = 0;
        fullscreenQuadVBO = nullptr;

        CCDirector::purgeDirector();
    }
};

std::optional<std::string> readResourceFile(const fs::path& path) {
    std::ifstream t(Mod::get()->getResourcesDir() / path);
    if (!t.is_open())
        return std::nullopt;

    t.seekg(0, std::ios::end);
    size_t size = t.tellg();
    std::string buffer(size, ' ');
    t.seekg(0);
    t.read(buffer.data(), size);
    t.close();

    return buffer;
}