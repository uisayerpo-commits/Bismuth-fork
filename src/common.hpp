#pragma once

#include "Geode/cocos/cocoa/CCGeometry.h"
#ifndef GEODE_ANDROID
#include "Geode/cocos/platform/win32/CCGL.h"
#endif
#include "ccTypes.h"
#include "glm/fwd.hpp"
#include <cstring>
#include <glm/glm.hpp>
#include <Geode/Geode.hpp>
#include <chrono>
#include <memory>
#include <stdint.h>

// On Android this is where the OpenGL ES functions get loaded (see es.hpp)
#include "es.hpp"

#ifdef __GNUC__
#define PACKED( __Declaration__ ) __Declaration__ __attribute__((__packed__))
#endif

#ifdef _MSC_VER
#define PACKED( __Declaration__ ) __pragma( pack(push, 1) ) __Declaration__ __pragma( pack(pop))
#endif

//#define DEBUG_LOG(...) log::info(__VA_ARGS__)
#define DEBUG_LOG(...)

namespace bismuth {
    /*
        True when the "Enable debug tools" setting is on. Everything that is only there for debugging
        (logging, diagnostics, profiling timers, the debug text) checks this. With it off nothing like
        that runs. It is read when the mod loads and again every time a level starts.
    */
    inline bool debugTools = false;
}

/*
    Logging for the mod. Same as geode::log, but it does nothing unless the debug tools are on.
    (A failure to start the renderer is silent too then, turn the debug tools on to see why.)
*/
namespace blog {
    template <typename... Args>
    inline void debug(fmt::format_string<Args...> format, Args&&... args) {
        if (bismuth::debugTools)
            geode::log::debug(format, std::forward<Args>(args)...);
    }

    template <typename... Args>
    inline void info(fmt::format_string<Args...> format, Args&&... args) {
        if (bismuth::debugTools)
            geode::log::info(format, std::forward<Args>(args)...);
    }

    template <typename... Args>
    inline void warn(fmt::format_string<Args...> format, Args&&... args) {
        if (bismuth::debugTools)
            geode::log::warn(format, std::forward<Args>(args)...);
    }

    template <typename... Args>
    inline void error(fmt::format_string<Args...> format, Args&&... args) {
        if (bismuth::debugTools)
            geode::log::error(format, std::forward<Args>(args)...);
    }
}

using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using i8  = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using vec2 = glm::vec2;
using vec3 = glm::vec3;
using vec4 = glm::vec4;

namespace fs = std::filesystem;

// #define BISMUTH_ASSERT(COND) { if (!(COND)) geode::log::error("{}:{}: Assertion failed: {}", fs::path(__FILE__).filename(), __LINE__, #COND); }
#define BISMUTH_ASSERT(COND)

// TODO: Figure out how to set usize to the pointer size of this machine's architecture
using usize = u64;
using isize = i64;

template <typename T>
using UPtr = std::unique_ptr<T>;

template <typename T, typename... Args>
inline std::unique_ptr<T> makeUnique(Args&& ...args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

inline vec2 ccPointToGLM(const cocos2d::CCPoint& point) {
    return vec2(point.x, point.y);
}

inline vec3 ccColor3BToGLM(const cocos2d::ccColor3B& color) {
    return vec3(color.r / 255.f, color.g / 255.f, color.b / 255.f);
}

extern std::chrono::steady_clock::time_point modStartTime;

inline u64 getTime() {
    return (std::chrono::high_resolution_clock::now() - modStartTime).count();
}

inline bool isNVidiaGPU() {
    return strcmp((const char*)glGetString(GL_VENDOR), "NVIDIA Corporation") == 0;
}

inline bool isAMDGPU() {
    return strcmp((const char*)glGetString(GL_VENDOR), "AMD") == 0;
}

inline bool isIntelGPU() {
    return strcmp((const char*)glGetString(GL_VENDOR), "Intel") == 0;
}

#ifdef GEODE_ANDROID

#ifndef APIENTRY
#define APIENTRY
#endif

/*
    The pieces of OpenGL 4.3 the renderer uses that are not in OpenGL ES 3.1 core's
    function names, wrapped so the rest of the code is the same on both platforms.
*/
namespace bgl {
    inline void dispatchCompute(GLuint x, GLuint y, GLuint z) {
        glDispatchCompute(x, y, z);
    }

    inline void memoryBarrier(GLbitfield barriers) {
        glMemoryBarrier(barriers);
    }

    /*
        OpenGL ES has no glMultiDrawElementsIndirect (GL_EXT_multi_draw_indirect is missing
        on the Mali GPUs). This does the same thing with one glDrawElementsIndirect per command.
    */
    inline void multiDrawElementsIndirect(GLenum mode, GLenum type, const void* indirect, GLsizei drawCount, GLsizei stride) {
        if (stride == 0)
            stride = 5 * sizeof(unsigned int);

        const char* commands = static_cast<const char*>(indirect);
        for (GLsizei i = 0; i < drawCount; i++)
            glDrawElementsIndirect(mode, type, commands + (usize)i * stride);
    }

    /*
        Checks that the current context can run the renderer (OpenGL ES 3.1 with
        compute shaders and indirect drawing). Needs a current GL context.
        Logs the reason when it fails.
    */
    bool init();

    // The version of the '#version' line of the shaders (310 es)
    u32 glslVersion();
}

#else

#define GL_BUFFER 0x82E0
#define GL_SHADER 0x82E1
#define GL_PROGRAM 0x82E2

#ifndef APIENTRY
#define APIENTRY
#endif

// Fallbacks for tokens that older GL headers may not define
#ifndef GL_COPY_READ_BUFFER
#define GL_COPY_READ_BUFFER 0x8F36
#endif
#ifndef GL_COPY_WRITE_BUFFER
#define GL_COPY_WRITE_BUFFER 0x8F37
#endif
#ifndef GL_ELEMENT_ARRAY_BARRIER_BIT
#define GL_ELEMENT_ARRAY_BARRIER_BIT 0x00000002
#endif
#ifndef GL_COMMAND_BARRIER_BIT
#define GL_COMMAND_BARRIER_BIT 0x00000040
#endif
#ifndef GL_SHADER_STORAGE_BARRIER_BIT
#define GL_SHADER_STORAGE_BARRIER_BIT 0x00002000
#endif

void glObjectLabel(GLenum id, GLuint name, GLsizei length, const char* label);

/*
    OpenGL entry points that the bundled GL headers don't expose.
    They are loaded at runtime by bgl::init().

    NOTE: The calling convention (APIENTRY) has to be part of the type.
          On 32-bit Windows GL functions are __stdcall, calling them
          through a plain function pointer type corrupts the stack.
*/
namespace bgl {
    using DispatchComputeProc            = void (APIENTRY*)(GLuint, GLuint, GLuint);
    using MultiDrawElementsIndirectProc  = void (APIENTRY*)(GLenum, GLenum, const void*, GLsizei, GLsizei);
    using MemoryBarrierProc              = void (APIENTRY*)(GLbitfield);

    extern DispatchComputeProc           dispatchCompute;
    extern MultiDrawElementsIndirectProc multiDrawElementsIndirect;
    extern MemoryBarrierProc             memoryBarrier;

    /*
        Checks that the current context can run the renderer (OpenGL 4.3+
        with compute shaders and indirect drawing) and loads the entry points
        above. Needs a current GL context. Logs the reason when it fails.
    */
    bool init();

    /*
        The GLSL version to put in the '#version' line of the shaders.
        This is the version of the GL context, capped at 460.
    */
    u32 glslVersion();
}


#endif // GEODE_ANDROID

/*
    Draws a fullscreen quad.
    VAO is handled automatically.
*/
void drawFullscreenQuad();

std::optional<std::string> readResourceFile(const fs::path& path);

inline glm::vec2 getClockwise(const glm::vec2& v) {
    return { v.y, -v.x };
}

inline glm::vec2 getCounterClockwise(const glm::vec2& v) {
    return { -v.y, v.x };
}