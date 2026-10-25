#pragma once

/*
    OpenGL ES function loading (Android).

    Geode's Android GL header only declares OpenGL ES 2.0 and renames some functions
    (glBindVertexArray -> glBindVertexArrayOES). This renderer needs ES 3.1 (compute
    shaders, SSBOs, indirect drawing). So every GL function the mod calls is looked up
    in the GL library at startup, and the glXxx names are macros for those pointers.
    The rest of the code keeps calling glBindBuffer(...) like before.

    Add a function here (loader, undef and define) when the code starts calling a new one.

    It has to be included after Geode.hpp (common.hpp does that), the macros below would
    break the declarations in the GL header otherwise.
*/

#ifdef GEODE_ANDROID

#include <dlfcn.h>
#include <cstddef>
#include <cstdint>
#include <initializer_list>

namespace es {

inline void* resolve(const char* name) {
    void* symbol = dlsym(RTLD_DEFAULT, name);
    if (symbol)
        return symbol;

    for (const char* library : { "libGLESv3.so", "libGLESv2.so" }) {
        if (void* handle = dlopen(library, RTLD_NOW)) {
            symbol = dlsym(handle, name);
            if (symbol)
                return symbol;
        }
    }
    return nullptr;
}

template <typename F>
inline F load(const char* name) {
    return reinterpret_cast<F>(resolve(name));
}

template <typename F>
inline F loadOr(const char* name, F fallback) {
    F function = load<F>(name);
    return function ? function : fallback;
}

// Stand-ins for what OpenGL ES doesn't have. The code that uses them is debug tooling.
namespace fb {
inline void ObjectLabel(unsigned, unsigned, int, const char*) {}
inline void GenQueries(int count, unsigned* ids) { for (int i = 0; i < count; i++) ids[i] = 0; }
inline void DeleteQueries(int, const unsigned*) {}
inline void BeginQuery(unsigned, unsigned) {}
inline void EndQuery(unsigned) {}
inline void GetQueryObjecti64v(unsigned, unsigned, int64_t* value) { *value = 0; }
inline void PolygonMode(unsigned, unsigned) {}
inline void VertexAttribLPointer(unsigned, int, unsigned, int, const void*) {}
inline void GetBufferSubData(unsigned, std::ptrdiff_t, std::ptrdiff_t, void*) {}
}

inline void (*DeleteShader)(unsigned) = load<void (*)(unsigned)>("glDeleteShader");
inline void (*GetIntegerv)(unsigned, int*) = load<void (*)(unsigned, int*)>("glGetIntegerv");
inline void (*TexParameteri)(unsigned, unsigned, int) = load<void (*)(unsigned, unsigned, int)>("glTexParameteri");
inline const unsigned char* (*GetString)(unsigned) = load<const unsigned char* (*)(unsigned)>("glGetString");
inline void (*BindVertexArray)(unsigned) = load<void (*)(unsigned)>("glBindVertexArray");
inline void (*BindTexture)(unsigned, unsigned) = load<void (*)(unsigned, unsigned)>("glBindTexture");
inline void (*ActiveTexture)(unsigned) = load<void (*)(unsigned)>("glActiveTexture");
inline void (*BindBuffer)(unsigned, unsigned) = load<void (*)(unsigned, unsigned)>("glBindBuffer");
inline void (*BindFramebuffer)(unsigned, unsigned) = load<void (*)(unsigned, unsigned)>("glBindFramebuffer");
inline void (*DeleteTextures)(int, const unsigned*) = load<void (*)(int, const unsigned*)>("glDeleteTextures");
inline void (*VertexAttribPointer)(unsigned, int, unsigned, unsigned char, int, const void*) = load<void (*)(unsigned, int, unsigned, unsigned char, int, const void*)>("glVertexAttribPointer");
inline void (*GetProgramiv)(unsigned, unsigned, int*) = load<void (*)(unsigned, unsigned, int*)>("glGetProgramiv");
inline void (*GenVertexArrays)(int, unsigned*) = load<void (*)(int, unsigned*)>("glGenVertexArrays");
inline void (*EnableVertexAttribArray)(unsigned) = load<void (*)(unsigned)>("glEnableVertexAttribArray");
inline void (*DeleteVertexArrays)(int, const unsigned*) = load<void (*)(int, const unsigned*)>("glDeleteVertexArrays");
inline void (*DeleteProgram)(unsigned) = load<void (*)(unsigned)>("glDeleteProgram");
inline void (*Clear)(unsigned) = load<void (*)(unsigned)>("glClear");
inline void (*AttachShader)(unsigned, unsigned) = load<void (*)(unsigned, unsigned)>("glAttachShader");
inline void (*UseProgram)(unsigned) = load<void (*)(unsigned)>("glUseProgram");
inline void (*Uniform1iv)(int, int, const int*) = load<void (*)(int, int, const int*)>("glUniform1iv");
inline void (*Uniform1i)(int, int) = load<void (*)(int, int)>("glUniform1i");
inline void (*TexImage2D)(unsigned, int, int, int, int, int, unsigned, unsigned, const void*) = load<void (*)(unsigned, int, int, int, int, int, unsigned, unsigned, const void*)>("glTexImage2D");
inline void (*StencilOp)(unsigned, unsigned, unsigned) = load<void (*)(unsigned, unsigned, unsigned)>("glStencilOp");
inline void (*LinkProgram)(unsigned) = load<void (*)(unsigned)>("glLinkProgram");
inline void (*GetShaderiv)(unsigned, unsigned, int*) = load<void (*)(unsigned, unsigned, int*)>("glGetShaderiv");
inline unsigned (*GetError)() = load<unsigned (*)()>("glGetError");
inline void (*GenTextures)(int, unsigned*) = load<void (*)(int, unsigned*)>("glGenTextures");
inline void (*Enable)(unsigned) = load<void (*)(unsigned)>("glEnable");
inline void (*DrawArrays)(unsigned, int, int) = load<void (*)(unsigned, int, int)>("glDrawArrays");
inline void (*DeleteFramebuffers)(int, const unsigned*) = load<void (*)(int, const unsigned*)>("glDeleteFramebuffers");
inline void (*DeleteBuffers)(int, const unsigned*) = load<void (*)(int, const unsigned*)>("glDeleteBuffers");
inline unsigned (*CreateProgram)() = load<unsigned (*)()>("glCreateProgram");
inline void (*BlendFunc)(unsigned, unsigned) = load<void (*)(unsigned, unsigned)>("glBlendFunc");
inline void (*BindBufferBase)(unsigned, unsigned, unsigned) = load<void (*)(unsigned, unsigned, unsigned)>("glBindBufferBase");
inline void (*VertexAttribIPointer)(unsigned, int, unsigned, int, const void*) = load<void (*)(unsigned, int, unsigned, int, const void*)>("glVertexAttribIPointer");
inline unsigned char (*UnmapBuffer)(unsigned) = load<unsigned char (*)(unsigned)>("glUnmapBuffer");
inline void (*UniformMatrix4fv)(int, int, unsigned char, const float*) = load<void (*)(int, int, unsigned char, const float*)>("glUniformMatrix4fv");
inline void (*Uniform4f)(int, float, float, float, float) = load<void (*)(int, float, float, float, float)>("glUniform4f");
inline void (*Uniform3f)(int, float, float, float) = load<void (*)(int, float, float, float)>("glUniform3f");
inline void (*Uniform2f)(int, float, float) = load<void (*)(int, float, float)>("glUniform2f");
inline void (*Uniform1ui)(int, unsigned) = load<void (*)(int, unsigned)>("glUniform1ui");
inline void (*Uniform1f)(int, float) = load<void (*)(int, float)>("glUniform1f");
inline void (*StencilFunc)(unsigned, int, unsigned) = load<void (*)(unsigned, int, unsigned)>("glStencilFunc");
inline void (*ShaderSource)(unsigned, int, const char* const*, const int*) = load<void (*)(unsigned, int, const char* const*, const int*)>("glShaderSource");
inline void (*ReadPixels)(int, int, int, int, unsigned, unsigned, void*) = load<void (*)(int, int, int, int, unsigned, unsigned, void*)>("glReadPixels");
inline void* (*MapBufferRange)(unsigned, std::ptrdiff_t, std::ptrdiff_t, unsigned) = load<void* (*)(unsigned, std::ptrdiff_t, std::ptrdiff_t, unsigned)>("glMapBufferRange");
inline int (*GetUniformLocation)(unsigned, const char*) = load<int (*)(unsigned, const char*)>("glGetUniformLocation");
inline void (*GetShaderInfoLog)(unsigned, int, int*, char*) = load<void (*)(unsigned, int, int*, char*)>("glGetShaderInfoLog");
inline void (*GetProgramInfoLog)(unsigned, int, int*, char*) = load<void (*)(unsigned, int, int*, char*)>("glGetProgramInfoLog");
inline void (*GenFramebuffers)(int, unsigned*) = load<void (*)(int, unsigned*)>("glGenFramebuffers");
inline void (*FramebufferTexture2D)(unsigned, unsigned, unsigned, unsigned, int) = load<void (*)(unsigned, unsigned, unsigned, unsigned, int)>("glFramebufferTexture2D");
inline void (*Disable)(unsigned) = load<void (*)(unsigned)>("glDisable");
inline unsigned (*CreateShader)(unsigned) = load<unsigned (*)(unsigned)>("glCreateShader");
inline void (*CompileShader)(unsigned) = load<void (*)(unsigned)>("glCompileShader");
inline void (*ClearStencil)(int) = load<void (*)(int)>("glClearStencil");
inline void (*BufferSubData)(unsigned, std::ptrdiff_t, std::ptrdiff_t, const void*) = load<void (*)(unsigned, std::ptrdiff_t, std::ptrdiff_t, const void*)>("glBufferSubData");
inline void (*BufferData)(unsigned, std::ptrdiff_t, const void*, unsigned) = load<void (*)(unsigned, std::ptrdiff_t, const void*, unsigned)>("glBufferData");
inline void (*BlendFuncSeparate)(unsigned, unsigned, unsigned, unsigned) = load<void (*)(unsigned, unsigned, unsigned, unsigned)>("glBlendFuncSeparate");
inline void (*GenBuffers)(int, unsigned*) = load<void (*)(int, unsigned*)>("glGenBuffers");
inline void (*TexStorage2D)(unsigned, int, unsigned, int, int) = load<void (*)(unsigned, int, unsigned, int, int)>("glTexStorage2D");
inline void (*BindImageTexture)(unsigned, unsigned, int, unsigned char, int, unsigned, unsigned) = load<void (*)(unsigned, unsigned, int, unsigned char, int, unsigned, unsigned)>("glBindImageTexture");
inline void (*DispatchCompute)(unsigned, unsigned, unsigned) = load<void (*)(unsigned, unsigned, unsigned)>("glDispatchCompute");
inline void (*MemoryBarrier)(unsigned) = load<void (*)(unsigned)>("glMemoryBarrier");
inline void (*DrawElementsIndirect)(unsigned, unsigned, const void*) = load<void (*)(unsigned, unsigned, const void*)>("glDrawElementsIndirect");
inline void (*ObjectLabel)(unsigned, unsigned, int, const char*) = loadOr<void (*)(unsigned, unsigned, int, const char*)>("glObjectLabel", fb::ObjectLabel);
inline void (*GenQueries)(int, unsigned*) = fb::GenQueries;
inline void (*DeleteQueries)(int, const unsigned*) = fb::DeleteQueries;
inline void (*BeginQuery)(unsigned, unsigned) = fb::BeginQuery;
inline void (*EndQuery)(unsigned) = fb::EndQuery;
inline void (*GetQueryObjecti64v)(unsigned, unsigned, int64_t*) = fb::GetQueryObjecti64v;
inline void (*PolygonMode)(unsigned, unsigned) = fb::PolygonMode;
inline void (*VertexAttribLPointer)(unsigned, int, unsigned, int, const void*) = fb::VertexAttribLPointer;
inline void (*GetBufferSubData)(unsigned, std::ptrdiff_t, std::ptrdiff_t, void*) = fb::GetBufferSubData;

} // namespace es

#undef glDeleteShader
#define glDeleteShader (*es::DeleteShader)
#undef glGetIntegerv
#define glGetIntegerv (*es::GetIntegerv)
#undef glTexParameteri
#define glTexParameteri (*es::TexParameteri)
#undef glGetString
#define glGetString (*es::GetString)
#undef glBindVertexArray
#define glBindVertexArray (*es::BindVertexArray)
#undef glBindTexture
#define glBindTexture (*es::BindTexture)
#undef glActiveTexture
#define glActiveTexture (*es::ActiveTexture)
#undef glBindBuffer
#define glBindBuffer (*es::BindBuffer)
#undef glBindFramebuffer
#define glBindFramebuffer (*es::BindFramebuffer)
#undef glDeleteTextures
#define glDeleteTextures (*es::DeleteTextures)
#undef glVertexAttribPointer
#define glVertexAttribPointer (*es::VertexAttribPointer)
#undef glGetProgramiv
#define glGetProgramiv (*es::GetProgramiv)
#undef glGenVertexArrays
#define glGenVertexArrays (*es::GenVertexArrays)
#undef glEnableVertexAttribArray
#define glEnableVertexAttribArray (*es::EnableVertexAttribArray)
#undef glDeleteVertexArrays
#define glDeleteVertexArrays (*es::DeleteVertexArrays)
#undef glDeleteProgram
#define glDeleteProgram (*es::DeleteProgram)
#undef glClear
#define glClear (*es::Clear)
#undef glAttachShader
#define glAttachShader (*es::AttachShader)
#undef glUseProgram
#define glUseProgram (*es::UseProgram)
#undef glUniform1iv
#define glUniform1iv (*es::Uniform1iv)
#undef glUniform1i
#define glUniform1i (*es::Uniform1i)
#undef glTexImage2D
#define glTexImage2D (*es::TexImage2D)
#undef glStencilOp
#define glStencilOp (*es::StencilOp)
#undef glLinkProgram
#define glLinkProgram (*es::LinkProgram)
#undef glGetShaderiv
#define glGetShaderiv (*es::GetShaderiv)
#undef glGetError
#define glGetError (*es::GetError)
#undef glGenTextures
#define glGenTextures (*es::GenTextures)
#undef glEnable
#define glEnable (*es::Enable)
#undef glDrawArrays
#define glDrawArrays (*es::DrawArrays)
#undef glDeleteFramebuffers
#define glDeleteFramebuffers (*es::DeleteFramebuffers)
#undef glDeleteBuffers
#define glDeleteBuffers (*es::DeleteBuffers)
#undef glCreateProgram
#define glCreateProgram (*es::CreateProgram)
#undef glBlendFunc
#define glBlendFunc (*es::BlendFunc)
#undef glBindBufferBase
#define glBindBufferBase (*es::BindBufferBase)
#undef glVertexAttribIPointer
#define glVertexAttribIPointer (*es::VertexAttribIPointer)
#undef glUnmapBuffer
#define glUnmapBuffer (*es::UnmapBuffer)
#undef glUniformMatrix4fv
#define glUniformMatrix4fv (*es::UniformMatrix4fv)
#undef glUniform4f
#define glUniform4f (*es::Uniform4f)
#undef glUniform3f
#define glUniform3f (*es::Uniform3f)
#undef glUniform2f
#define glUniform2f (*es::Uniform2f)
#undef glUniform1ui
#define glUniform1ui (*es::Uniform1ui)
#undef glUniform1f
#define glUniform1f (*es::Uniform1f)
#undef glStencilFunc
#define glStencilFunc (*es::StencilFunc)
#undef glShaderSource
#define glShaderSource (*es::ShaderSource)
#undef glReadPixels
#define glReadPixels (*es::ReadPixels)
#undef glMapBufferRange
#define glMapBufferRange (*es::MapBufferRange)
#undef glGetUniformLocation
#define glGetUniformLocation (*es::GetUniformLocation)
#undef glGetShaderInfoLog
#define glGetShaderInfoLog (*es::GetShaderInfoLog)
#undef glGetProgramInfoLog
#define glGetProgramInfoLog (*es::GetProgramInfoLog)
#undef glGenFramebuffers
#define glGenFramebuffers (*es::GenFramebuffers)
#undef glFramebufferTexture2D
#define glFramebufferTexture2D (*es::FramebufferTexture2D)
#undef glDisable
#define glDisable (*es::Disable)
#undef glCreateShader
#define glCreateShader (*es::CreateShader)
#undef glCompileShader
#define glCompileShader (*es::CompileShader)
#undef glClearStencil
#define glClearStencil (*es::ClearStencil)
#undef glBufferSubData
#define glBufferSubData (*es::BufferSubData)
#undef glBufferData
#define glBufferData (*es::BufferData)
#undef glBlendFuncSeparate
#define glBlendFuncSeparate (*es::BlendFuncSeparate)
#undef glGenBuffers
#define glGenBuffers (*es::GenBuffers)
#undef glTexStorage2D
#define glTexStorage2D (*es::TexStorage2D)
#undef glBindImageTexture
#define glBindImageTexture (*es::BindImageTexture)
#undef glDispatchCompute
#define glDispatchCompute (*es::DispatchCompute)
#undef glMemoryBarrier
#define glMemoryBarrier (*es::MemoryBarrier)
#undef glDrawElementsIndirect
#define glDrawElementsIndirect (*es::DrawElementsIndirect)
#undef glObjectLabel
#define glObjectLabel (*es::ObjectLabel)
#undef glGenQueries
#define glGenQueries (*es::GenQueries)
#undef glDeleteQueries
#define glDeleteQueries (*es::DeleteQueries)
#undef glBeginQuery
#define glBeginQuery (*es::BeginQuery)
#undef glEndQuery
#define glEndQuery (*es::EndQuery)
#undef glGetQueryObjecti64v
#define glGetQueryObjecti64v (*es::GetQueryObjecti64v)
#undef glPolygonMode
#define glPolygonMode (*es::PolygonMode)
#undef glVertexAttribLPointer
#define glVertexAttribLPointer (*es::VertexAttribLPointer)
#undef glGetBufferSubData
#define glGetBufferSubData (*es::GetBufferSubData)

// Tokens the OpenGL ES 2.0 header doesn't have
#ifndef GL_BUFFER
#define GL_BUFFER 0x82E0
#endif
#ifndef GL_SHADER
#define GL_SHADER 0x82E1
#endif
#ifndef GL_PROGRAM
#define GL_PROGRAM 0x82E2
#endif
#ifndef GL_COPY_READ_BUFFER
#define GL_COPY_READ_BUFFER 0x8F36
#endif
#ifndef GL_COPY_WRITE_BUFFER
#define GL_COPY_WRITE_BUFFER 0x8F37
#endif
#ifndef GL_UNIFORM_BUFFER
#define GL_UNIFORM_BUFFER 0x8A11
#endif
#ifndef GL_SHADER_STORAGE_BUFFER
#define GL_SHADER_STORAGE_BUFFER 0x90D2
#endif
#ifndef GL_DRAW_INDIRECT_BUFFER
#define GL_DRAW_INDIRECT_BUFFER 0x8F3F
#endif
#ifndef GL_COMPUTE_SHADER
#define GL_COMPUTE_SHADER 0x91B9
#endif
#ifndef GL_DYNAMIC_COPY
#define GL_DYNAMIC_COPY 0x88EA
#endif
#ifndef GL_MAP_WRITE_BIT
#define GL_MAP_WRITE_BIT 0x0002
#endif
#ifndef GL_MAP_INVALIDATE_BUFFER_BIT
#define GL_MAP_INVALIDATE_BUFFER_BIT 0x0008
#endif
#ifndef GL_VERTEX_ARRAY_BINDING
#define GL_VERTEX_ARRAY_BINDING 0x85B5
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
#ifndef GL_RED
#define GL_RED 0x1903
#endif
#ifndef GL_MAP_READ_BIT
#define GL_MAP_READ_BIT 0x0001
#endif
#ifndef GL_BUFFER_UPDATE_BARRIER_BIT
#define GL_BUFFER_UPDATE_BARRIER_BIT 0x00000200
#endif
#ifndef GL_RGBA32F
#define GL_RGBA32F 0x8814
#endif
#ifndef GL_WRITE_ONLY
#define GL_WRITE_ONLY 0x88B9
#endif
#ifndef GL_TEXTURE_FETCH_BARRIER_BIT
#define GL_TEXTURE_FETCH_BARRIER_BIT 0x00000008
#endif
#ifndef GL_STENCIL_INDEX
#define GL_STENCIL_INDEX 0x1901
#endif
#ifndef GL_TIME_ELAPSED
#define GL_TIME_ELAPSED 0x88BF
#endif
#ifndef GL_QUERY_RESULT
#define GL_QUERY_RESULT 0x8866
#endif
#ifndef GL_DOUBLE
#define GL_DOUBLE 0x140A
#endif
#ifndef GL_FILL
#define GL_FILL 0x1B02
#endif
#ifndef GL_LINE
#define GL_LINE 0x1B01
#endif
#ifndef GL_FRONT_AND_BACK
#define GL_FRONT_AND_BACK 0x0408
#endif

#endif // GEODE_ANDROID
