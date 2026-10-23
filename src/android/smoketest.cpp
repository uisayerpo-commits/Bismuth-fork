#include "smoketest.hpp"

#include <dlfcn.h>

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

namespace {

// Everything is written out here instead of including a GL header, see probe.cpp for why
constexpr unsigned GL_VERTEX_SHADER_      = 0x8B31;
constexpr unsigned GL_FRAGMENT_SHADER_    = 0x8B30;
constexpr unsigned GL_COMPUTE_SHADER_     = 0x91B9;
constexpr unsigned GL_COMPILE_STATUS_     = 0x8B81;
constexpr unsigned GL_LINK_STATUS_        = 0x8B82;
constexpr unsigned GL_INFO_LOG_LENGTH_    = 0x8B84;
constexpr unsigned GL_ARRAY_BUFFER_       = 0x8892;
constexpr unsigned GL_ELEMENT_ARRAY_BUFFER_ = 0x8893;
constexpr unsigned GL_SHADER_STORAGE_BUFFER_ = 0x90D2;
constexpr unsigned GL_DRAW_INDIRECT_BUFFER_  = 0x8F3F;
constexpr unsigned GL_STATIC_DRAW_        = 0x88E4;
constexpr unsigned GL_DYNAMIC_DRAW_       = 0x88E8;
constexpr unsigned GL_DYNAMIC_COPY_       = 0x88EA;
constexpr unsigned GL_MAP_READ_BIT_       = 0x0001;
constexpr unsigned GL_TEXTURE_2D_         = 0x0DE1;
constexpr unsigned GL_TEXTURE0_           = 0x84C0;
constexpr unsigned GL_RGBA32F_            = 0x8814;
constexpr unsigned GL_RGBA8_              = 0x8058;
constexpr unsigned GL_TEXTURE_MIN_FILTER_ = 0x2801;
constexpr unsigned GL_TEXTURE_MAG_FILTER_ = 0x2800;
constexpr unsigned GL_TEXTURE_WRAP_S_     = 0x2802;
constexpr unsigned GL_TEXTURE_WRAP_T_     = 0x2803;
constexpr int      GL_NEAREST_            = 0x2600;
constexpr int      GL_CLAMP_TO_EDGE_      = 0x812F;
constexpr unsigned GL_WRITE_ONLY_         = 0x88B9;
constexpr unsigned GL_FRAMEBUFFER_        = 0x8D40;
constexpr unsigned GL_COLOR_ATTACHMENT0_  = 0x8CE0;
constexpr unsigned GL_FRAMEBUFFER_COMPLETE_ = 0x8CD5;
constexpr unsigned GL_COLOR_BUFFER_BIT_   = 0x4000;
constexpr unsigned GL_FLOAT_              = 0x1406;
constexpr unsigned GL_UNSIGNED_INT_       = 0x1405;
constexpr unsigned GL_UNSIGNED_BYTE_      = 0x1401;
constexpr unsigned GL_TRIANGLES_          = 0x0004;
constexpr unsigned GL_RGBA_               = 0x1908;
constexpr unsigned GL_SCISSOR_TEST_       = 0x0C11;

constexpr unsigned GL_FRAMEBUFFER_BINDING_  = 0x8CA6;
constexpr unsigned GL_VERTEX_ARRAY_BINDING_ = 0x85B5;
constexpr unsigned GL_CURRENT_PROGRAM_      = 0x8B8D;
constexpr unsigned GL_VIEWPORT_             = 0x0BA2;
constexpr unsigned GL_ACTIVE_TEXTURE_       = 0x84E0;
constexpr unsigned GL_TEXTURE_BINDING_2D_   = 0x8069;
constexpr unsigned GL_ARRAY_BUFFER_BINDING_ = 0x8894;
constexpr unsigned GL_DRAW_INDIRECT_BUFFER_BINDING_ = 0x8F43;
constexpr unsigned GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS_ = 0x8B4C;

// Memory barrier bits
constexpr unsigned BARRIER_ELEMENT_ARRAY = 0x0002;
constexpr unsigned BARRIER_TEXTURE_FETCH = 0x0008;
constexpr unsigned BARRIER_COMMAND       = 0x0040;
constexpr unsigned BARRIER_SSBO          = 0x2000;

struct GL {
    unsigned (*GetError)();
    void (*GetIntegerv)(unsigned, int*);
    unsigned char (*IsEnabled)(unsigned);
    void (*Enable)(unsigned);
    void (*Disable)(unsigned);

    unsigned (*CreateShader)(unsigned);
    void (*ShaderSource)(unsigned, int, const char* const*, const int*);
    void (*CompileShader)(unsigned);
    void (*GetShaderiv)(unsigned, unsigned, int*);
    void (*GetShaderInfoLog)(unsigned, int, int*, char*);
    void (*DeleteShader)(unsigned);
    unsigned (*CreateProgram)();
    void (*AttachShader)(unsigned, unsigned);
    void (*LinkProgram)(unsigned);
    void (*GetProgramiv)(unsigned, unsigned, int*);
    void (*GetProgramInfoLog)(unsigned, int, int*, char*);
    void (*DeleteProgram)(unsigned);
    void (*UseProgram)(unsigned);
    int (*GetUniformLocation)(unsigned, const char*);
    void (*Uniform1i)(int, int);
    void (*Uniform4f)(int, float, float, float, float);

    void (*GenBuffers)(int, unsigned*);
    void (*BindBuffer)(unsigned, unsigned);
    void (*BufferData)(unsigned, std::ptrdiff_t, const void*, unsigned);
    void (*DeleteBuffers)(int, const unsigned*);
    void (*BindBufferBase)(unsigned, unsigned, unsigned);
    void* (*MapBufferRange)(unsigned, std::ptrdiff_t, std::ptrdiff_t, unsigned);
    unsigned char (*UnmapBuffer)(unsigned);

    void (*DispatchCompute)(unsigned, unsigned, unsigned);
    void (*MemoryBarrier)(unsigned);

    void (*GenTextures)(int, unsigned*);
    void (*BindTexture)(unsigned, unsigned);
    void (*TexStorage2D)(unsigned, int, unsigned, int, int);
    void (*TexParameteri)(unsigned, unsigned, int);
    void (*DeleteTextures)(int, const unsigned*);
    void (*ActiveTexture)(unsigned);
    void (*BindImageTexture)(unsigned, unsigned, int, unsigned char, int, unsigned, unsigned);

    void (*GenFramebuffers)(int, unsigned*);
    void (*BindFramebuffer)(unsigned, unsigned);
    void (*FramebufferTexture2D)(unsigned, unsigned, unsigned, unsigned, int);
    unsigned (*CheckFramebufferStatus)(unsigned);
    void (*DeleteFramebuffers)(int, const unsigned*);
    void (*Viewport)(int, int, int, int);
    void (*ClearColor)(float, float, float, float);
    void (*Clear)(unsigned);
    void (*ColorMask)(unsigned char, unsigned char, unsigned char, unsigned char);

    void (*GenVertexArrays)(int, unsigned*);
    void (*BindVertexArray)(unsigned);
    void (*DeleteVertexArrays)(int, const unsigned*);
    void (*EnableVertexAttribArray)(unsigned);
    void (*VertexAttribPointer)(unsigned, int, unsigned, unsigned char, int, const void*);
    void (*VertexAttribIPointer)(unsigned, int, unsigned, int, const void*);
    void (*DrawElementsIndirect)(unsigned, unsigned, const void*);
    void (*ReadPixels)(int, int, int, int, unsigned, unsigned, void*);
};

GL gl;

void* findGLSymbol(const char* name) {
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

std::string loadFunctions() {
    std::string missing;

    #define BIS_LOAD(NAME) do { \
        gl.NAME = reinterpret_cast<decltype(gl.NAME)>(findGLSymbol("gl" #NAME)); \
        if (!gl.NAME) missing += " gl" #NAME; \
    } while (0)

    BIS_LOAD(GetError); BIS_LOAD(GetIntegerv); BIS_LOAD(IsEnabled); BIS_LOAD(Enable); BIS_LOAD(Disable);
    BIS_LOAD(CreateShader); BIS_LOAD(ShaderSource); BIS_LOAD(CompileShader); BIS_LOAD(GetShaderiv);
    BIS_LOAD(GetShaderInfoLog); BIS_LOAD(DeleteShader); BIS_LOAD(CreateProgram); BIS_LOAD(AttachShader);
    BIS_LOAD(LinkProgram); BIS_LOAD(GetProgramiv); BIS_LOAD(GetProgramInfoLog); BIS_LOAD(DeleteProgram);
    BIS_LOAD(UseProgram); BIS_LOAD(GetUniformLocation); BIS_LOAD(Uniform1i); BIS_LOAD(Uniform4f);
    BIS_LOAD(GenBuffers); BIS_LOAD(BindBuffer); BIS_LOAD(BufferData); BIS_LOAD(DeleteBuffers);
    BIS_LOAD(BindBufferBase); BIS_LOAD(MapBufferRange); BIS_LOAD(UnmapBuffer);
    BIS_LOAD(DispatchCompute); BIS_LOAD(MemoryBarrier);
    BIS_LOAD(GenTextures); BIS_LOAD(BindTexture); BIS_LOAD(TexStorage2D); BIS_LOAD(TexParameteri);
    BIS_LOAD(DeleteTextures); BIS_LOAD(ActiveTexture); BIS_LOAD(BindImageTexture);
    BIS_LOAD(GenFramebuffers); BIS_LOAD(BindFramebuffer); BIS_LOAD(FramebufferTexture2D);
    BIS_LOAD(CheckFramebufferStatus); BIS_LOAD(DeleteFramebuffers); BIS_LOAD(Viewport);
    BIS_LOAD(ClearColor); BIS_LOAD(Clear); BIS_LOAD(ColorMask);
    BIS_LOAD(GenVertexArrays); BIS_LOAD(BindVertexArray); BIS_LOAD(DeleteVertexArrays);
    BIS_LOAD(EnableVertexAttribArray); BIS_LOAD(VertexAttribPointer); BIS_LOAD(VertexAttribIPointer);
    BIS_LOAD(DrawElementsIndirect); BIS_LOAD(ReadPixels);

    #undef BIS_LOAD
    return missing;
}

// The test uses N sprites on a 16 x 16 grid. Every sprite covers 2 x 2 pixels
// of a 64 x 64 image, so the number of drawn pixels is exact.
constexpr unsigned SPRITE_COUNT = 256;
constexpr unsigned LAYER_COUNT  = 2;
constexpr unsigned IMAGE_SIZE   = 64;
constexpr unsigned INDICES_PER_SPRITE = 6;
constexpr unsigned COMMAND_SIZE = 20; // 5 unsigned ints

const char* CULL_SHADER = R"(#version 310 es
precision highp float;
precision highp int;

layout(local_size_x = 64) in;

layout(std430, binding = 0) buffer Counters { uint counters[2]; };
layout(std430, binding = 1) buffer Indices { uint indices[]; };
layout(rgba32f, binding = 0) writeonly uniform highp image2D u_sprites;

void main() {
    uint i = gl_GlobalInvocationID.x;
    if (i >= 256u)
        return;

    // Sprite data: x and y in pixels, size in pixels
    imageStore(u_sprites, ivec2(int(i), 0), vec4(float(i % 16u) * 4.0 + 1.0, float(i / 16u) * 4.0 + 1.0, 2.0, 1.0));

    // "Culled" sprites
    if (i % 3u == 0u)
        return;

    uint layer = i % 2u;
    uint slot = atomicAdd(counters[layer], 1u);

    uint base = layer * 256u * 6u + slot * 6u;
    uint v = i * 4u;
    indices[base + 0u] = v;
    indices[base + 1u] = v + 1u;
    indices[base + 2u] = v + 2u;
    indices[base + 3u] = v + 2u;
    indices[base + 4u] = v + 1u;
    indices[base + 5u] = v + 3u;
}
)";

const char* FINALIZE_SHADER = R"(#version 310 es
precision highp float;
precision highp int;

layout(local_size_x = 2) in;

struct Command {
    uint count;
    uint instanceCount;
    uint firstIndex;
    uint baseVertex;
    uint baseInstance;
};

layout(std430, binding = 0) readonly buffer Counters { uint counters[2]; };
layout(std430, binding = 2) writeonly buffer Commands { Command commands[]; };

void main() {
    uint layer = gl_GlobalInvocationID.x;
    if (layer >= 2u)
        return;

    commands[layer] = Command(counters[layer] * 6u, 1u, layer * 256u * 6u, 0u, 0u);
}
)";

const char* DRAW_VERTEX_SHADER = R"(#version 310 es
precision highp float;
precision highp int;

layout(location = 0) in vec2 a_corner;
layout(location = 1) in uint a_sprite;

uniform highp sampler2D u_sprites;

void main() {
    vec4 sprite = texelFetch(u_sprites, ivec2(int(a_sprite), 0), 0);
    vec2 position = sprite.xy + a_corner * sprite.z;
    gl_Position = vec4(position / 64.0 * 2.0 - 1.0, 0.0, 1.0);
}
)";

const char* DRAW_FRAGMENT_SHADER = R"(#version 310 es
precision highp float;

uniform vec4 u_color;
out vec4 o_color;

void main() {
    o_color = u_color;
}
)";

struct VertexData {
    float x, y;
    uint32_t sprite;
};

struct SavedState {
    int framebuffer = 0;
    int vertexArray = 0;
    int program = 0;
    int arrayBuffer = 0;
    int indirectBuffer = 0;
    int activeTexture = GL_TEXTURE0_;
    int textureUnit15 = 0;
    int viewport[4] = { 0, 0, 0, 0 };
    bool scissor = false;

    void save() {
        gl.GetIntegerv(GL_FRAMEBUFFER_BINDING_, &framebuffer);
        gl.GetIntegerv(GL_VERTEX_ARRAY_BINDING_, &vertexArray);
        gl.GetIntegerv(GL_CURRENT_PROGRAM_, &program);
        gl.GetIntegerv(GL_ARRAY_BUFFER_BINDING_, &arrayBuffer);
        gl.GetIntegerv(GL_DRAW_INDIRECT_BUFFER_BINDING_, &indirectBuffer);
        gl.GetIntegerv(GL_VIEWPORT_, viewport);
        gl.GetIntegerv(GL_ACTIVE_TEXTURE_, &activeTexture);

        // Texture unit 15 is the only one the test touches, so the game's textures stay bound
        gl.ActiveTexture(GL_TEXTURE0_ + 15);
        gl.GetIntegerv(GL_TEXTURE_BINDING_2D_, &textureUnit15);
        gl.ActiveTexture(activeTexture);

        scissor = gl.IsEnabled(GL_SCISSOR_TEST_) != 0;
    }

    void restore() {
        gl.ActiveTexture(GL_TEXTURE0_ + 15);
        gl.BindTexture(GL_TEXTURE_2D_, textureUnit15);
        gl.ActiveTexture(activeTexture);

        gl.BindFramebuffer(GL_FRAMEBUFFER_, framebuffer);
        gl.Viewport(viewport[0], viewport[1], viewport[2], viewport[3]);
        gl.UseProgram(program);
        gl.BindVertexArray(vertexArray);
        gl.BindBuffer(GL_ARRAY_BUFFER_, arrayBuffer);
        gl.BindBuffer(GL_DRAW_INDIRECT_BUFFER_, indirectBuffer);

        if (scissor)
            gl.Enable(GL_SCISSOR_TEST_);
        else
            gl.Disable(GL_SCISSOR_TEST_);
    }
};

std::string hex(unsigned value) {
    char text[16];
    snprintf(text, sizeof(text), "0x%x", value);
    return text;
}

} // namespace

std::string runGlesSmokeTest(bool& passed, const std::function<void(const std::string&)>& progress) {
    passed = false;

    std::string report = "OpenGL ES smoke test\n";
    bool failed = false;

    auto note = [&](const std::string& line) {
        report += line + "\n";
        if (progress)
            progress(report);
    };

    auto fail = [&](const std::string& line) {
        failed = true;
        note("FAIL  " + line);
    };

    auto ok = [&](const std::string& line) {
        note("ok    " + line);
    };

    // Reports (and clears) every pending GL error
    auto checkErrors = [&](const char* where) {
        bool any = false;
        for (int i = 0; i < 16; i++) {
            unsigned error = gl.GetError();
            if (error == 0)
                break;
            fail(std::string("GL error ") + hex(error) + " after " + where);
            any = true;
        }
        return !any;
    };

    std::string missing = loadFunctions();
    if (!missing.empty()) {
        note("FAIL  missing GL functions:" + missing);
        note("RESULT: FAIL");
        return report;
    }

    note("started (if the report ends here, the driver crashed on the next step)");

    SavedState saved;
    saved.save();
    while (gl.GetError() != 0) {} // errors left by the game

    int vertexTextureUnits = -1;
    gl.GetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS_, &vertexTextureUnits);
    note("vertex texture units: " + std::to_string(vertexTextureUnits));
    checkErrors("queries");

    // Shader programs
    auto buildProgram = [&](const char* name, std::vector<std::pair<unsigned, const char*>> sources) -> unsigned {
        unsigned program = gl.CreateProgram();
        std::vector<unsigned> shaders;
        bool good = true;

        for (auto& [type, code] : sources) {
            unsigned shader = gl.CreateShader(type);
            gl.ShaderSource(shader, 1, &code, nullptr);
            gl.CompileShader(shader);

            int status = 0;
            gl.GetShaderiv(shader, GL_COMPILE_STATUS_, &status);
            if (!status) {
                int length = 0;
                gl.GetShaderiv(shader, GL_INFO_LOG_LENGTH_, &length);
                std::string log(length > 1 ? length : 1, '\0');
                gl.GetShaderInfoLog(shader, (int)log.size(), nullptr, log.data());
                fail(std::string(name) + " failed to compile: " + log.c_str());
                good = false;
            }
            gl.AttachShader(program, shader);
            shaders.push_back(shader);
        }

        if (good) {
            gl.LinkProgram(program);
            int status = 0;
            gl.GetProgramiv(program, GL_LINK_STATUS_, &status);
            if (!status) {
                int length = 0;
                gl.GetProgramiv(program, GL_INFO_LOG_LENGTH_, &length);
                std::string log(length > 1 ? length : 1, '\0');
                gl.GetProgramInfoLog(program, (int)log.size(), nullptr, log.data());
                fail(std::string(name) + " failed to link: " + log.c_str());
                good = false;
            }
        }

        for (unsigned shader : shaders)
            gl.DeleteShader(shader);

        if (!good) {
            gl.DeleteProgram(program);
            return 0;
        }

        ok(std::string(name) + " compiled and linked");
        return program;
    };

    unsigned cullProgram     = buildProgram("culling compute shader", { { GL_COMPUTE_SHADER_, CULL_SHADER } });
    unsigned finalizeProgram = buildProgram("draw command compute shader", { { GL_COMPUTE_SHADER_, FINALIZE_SHADER } });
    unsigned drawProgram     = buildProgram("draw program", { { GL_VERTEX_SHADER_, DRAW_VERTEX_SHADER }, { GL_FRAGMENT_SHADER_, DRAW_FRAGMENT_SHADER } });
    checkErrors("shader setup");

    // GL objects, deleted at the end whatever happens
    unsigned buffers[4] = { 0, 0, 0, 0 }; // counters, indices, commands, vertices
    unsigned textures[2] = { 0, 0 };      // sprite data, render target
    unsigned framebuffer = 0;
    unsigned vertexArray = 0;

    std::vector<uint8_t> pixels(IMAGE_SIZE * IMAGE_SIZE * 4, 0);
    unsigned counterResult[2] = { 0xFFFFFFFF, 0xFFFFFFFF };
    unsigned commandResult[LAYER_COUNT * 5] = {};

    if (!failed) {
        // Buffers
        gl.GenBuffers(4, buffers);

        unsigned zeroCounters[2] = { 0, 0 };
        gl.BindBuffer(GL_SHADER_STORAGE_BUFFER_, buffers[0]);
        gl.BufferData(GL_SHADER_STORAGE_BUFFER_, sizeof(zeroCounters), zeroCounters, GL_DYNAMIC_COPY_);

        gl.BindBuffer(GL_SHADER_STORAGE_BUFFER_, buffers[1]);
        gl.BufferData(GL_SHADER_STORAGE_BUFFER_, LAYER_COUNT * SPRITE_COUNT * INDICES_PER_SPRITE * 4, nullptr, GL_DYNAMIC_DRAW_);

        gl.BindBuffer(GL_SHADER_STORAGE_BUFFER_, buffers[2]);
        gl.BufferData(GL_SHADER_STORAGE_BUFFER_, LAYER_COUNT * COMMAND_SIZE, nullptr, GL_DYNAMIC_DRAW_);

        std::vector<VertexData> vertices;
        vertices.reserve(SPRITE_COUNT * 4);
        for (unsigned sprite = 0; sprite < SPRITE_COUNT; sprite++) {
            for (unsigned corner = 0; corner < 4; corner++)
                vertices.push_back({ (float)(corner & 1), (float)(corner >> 1), sprite });
        }
        gl.BindBuffer(GL_ARRAY_BUFFER_, buffers[3]);
        gl.BufferData(GL_ARRAY_BUFFER_, vertices.size() * sizeof(VertexData), vertices.data(), GL_STATIC_DRAW_);

        if (checkErrors("creating buffers"))
            ok("buffers created");

        // Textures: the sprite data (written by compute, read by the vertex shader) and the render target
        gl.GenTextures(2, textures);
        gl.ActiveTexture(GL_TEXTURE0_ + 15);

        gl.BindTexture(GL_TEXTURE_2D_, textures[0]);
        gl.TexStorage2D(GL_TEXTURE_2D_, 1, GL_RGBA32F_, SPRITE_COUNT, 1);
        gl.TexParameteri(GL_TEXTURE_2D_, GL_TEXTURE_MIN_FILTER_, GL_NEAREST_);
        gl.TexParameteri(GL_TEXTURE_2D_, GL_TEXTURE_MAG_FILTER_, GL_NEAREST_);
        gl.TexParameteri(GL_TEXTURE_2D_, GL_TEXTURE_WRAP_S_, GL_CLAMP_TO_EDGE_);
        gl.TexParameteri(GL_TEXTURE_2D_, GL_TEXTURE_WRAP_T_, GL_CLAMP_TO_EDGE_);

        gl.BindTexture(GL_TEXTURE_2D_, textures[1]);
        gl.TexStorage2D(GL_TEXTURE_2D_, 1, GL_RGBA8_, IMAGE_SIZE, IMAGE_SIZE);
        gl.TexParameteri(GL_TEXTURE_2D_, GL_TEXTURE_MIN_FILTER_, GL_NEAREST_);
        gl.TexParameteri(GL_TEXTURE_2D_, GL_TEXTURE_MAG_FILTER_, GL_NEAREST_);

        gl.GenFramebuffers(1, &framebuffer);
        gl.BindFramebuffer(GL_FRAMEBUFFER_, framebuffer);
        gl.FramebufferTexture2D(GL_FRAMEBUFFER_, GL_COLOR_ATTACHMENT0_, GL_TEXTURE_2D_, textures[1], 0);

        unsigned status = gl.CheckFramebufferStatus(GL_FRAMEBUFFER_);
        if (status != GL_FRAMEBUFFER_COMPLETE_)
            fail("framebuffer is incomplete: " + hex(status));
        if (checkErrors("creating textures and framebuffer") && status == GL_FRAMEBUFFER_COMPLETE_)
            ok("RGBA32F sprite texture and RGBA8 render target created");
    }

    if (!failed) {
        // Step 1: culling + compaction + sprite data
        gl.UseProgram(cullProgram);
        gl.BindBufferBase(GL_SHADER_STORAGE_BUFFER_, 0, buffers[0]);
        gl.BindBufferBase(GL_SHADER_STORAGE_BUFFER_, 1, buffers[1]);
        gl.BindImageTexture(0, textures[0], 0, 0, 0, GL_WRITE_ONLY_, GL_RGBA32F_);
        gl.DispatchCompute(SPRITE_COUNT / 64, 1, 1);
        gl.MemoryBarrier(BARRIER_SSBO);
        if (checkErrors("culling dispatch"))
            ok("culling dispatch ran (64 invocations per group)");

        // Step 2: draw commands from the counters
        gl.UseProgram(finalizeProgram);
        gl.BindBufferBase(GL_SHADER_STORAGE_BUFFER_, 0, buffers[0]);
        gl.BindBufferBase(GL_SHADER_STORAGE_BUFFER_, 2, buffers[2]);
        gl.DispatchCompute(1, 1, 1);
        gl.MemoryBarrier(BARRIER_SSBO | BARRIER_TEXTURE_FETCH | BARRIER_ELEMENT_ARRAY | BARRIER_COMMAND);
        if (checkErrors("draw command dispatch"))
            ok("draw command dispatch ran");
    }

    if (!failed) {
        // Step 3: one indirect draw per layer
        gl.BindFramebuffer(GL_FRAMEBUFFER_, framebuffer);
        gl.Viewport(0, 0, IMAGE_SIZE, IMAGE_SIZE);
        gl.Disable(GL_SCISSOR_TEST_);
        gl.ColorMask(1, 1, 1, 1);
        gl.ClearColor(0.f, 0.f, 0.f, 0.f);
        gl.Clear(GL_COLOR_BUFFER_BIT_);

        gl.GenVertexArrays(1, &vertexArray);
        gl.BindVertexArray(vertexArray);

        gl.BindBuffer(GL_ARRAY_BUFFER_, buffers[3]);
        gl.EnableVertexAttribArray(0);
        gl.VertexAttribPointer(0, 2, GL_FLOAT_, 0, sizeof(VertexData), (const void*)0);
        gl.EnableVertexAttribArray(1);
        gl.VertexAttribIPointer(1, 1, GL_UNSIGNED_INT_, sizeof(VertexData), (const void*)(uintptr_t)8);

        // The index buffer is the very buffer the compute shader wrote
        gl.BindBuffer(GL_ELEMENT_ARRAY_BUFFER_, buffers[1]);
        gl.BindBuffer(GL_DRAW_INDIRECT_BUFFER_, buffers[2]);

        gl.UseProgram(drawProgram);
        gl.ActiveTexture(GL_TEXTURE0_ + 15);
        gl.BindTexture(GL_TEXTURE_2D_, textures[0]);
        gl.Uniform1i(gl.GetUniformLocation(drawProgram, "u_sprites"), 15);

        int colorLocation = gl.GetUniformLocation(drawProgram, "u_color");
        for (unsigned layer = 0; layer < LAYER_COUNT; layer++) {
            if (layer == 0)
                gl.Uniform4f(colorLocation, 1.f, 0.f, 0.f, 1.f); // red
            else
                gl.Uniform4f(colorLocation, 0.f, 0.f, 1.f, 1.f); // blue

            gl.DrawElementsIndirect(GL_TRIANGLES_, GL_UNSIGNED_INT_, (const void*)(uintptr_t)(layer * COMMAND_SIZE));
        }

        if (checkErrors("indirect draws"))
            ok("2 glDrawElementsIndirect calls ran (one per layer)");

        gl.ReadPixels(0, 0, IMAGE_SIZE, IMAGE_SIZE, GL_RGBA_, GL_UNSIGNED_BYTE_, pixels.data());
        if (checkErrors("reading the pixels back"))
            ok("pixels read back");

        // Counters and commands, to see what the compute shaders produced
        gl.BindBuffer(GL_SHADER_STORAGE_BUFFER_, buffers[0]);
        if (void* data = gl.MapBufferRange(GL_SHADER_STORAGE_BUFFER_, 0, sizeof(counterResult), GL_MAP_READ_BIT_)) {
            memcpy(counterResult, data, sizeof(counterResult));
            gl.UnmapBuffer(GL_SHADER_STORAGE_BUFFER_);
        } else
            fail("could not map the counter buffer");

        gl.BindBuffer(GL_SHADER_STORAGE_BUFFER_, buffers[2]);
        if (void* data = gl.MapBufferRange(GL_SHADER_STORAGE_BUFFER_, 0, sizeof(commandResult), GL_MAP_READ_BIT_)) {
            memcpy(commandResult, data, sizeof(commandResult));
            gl.UnmapBuffer(GL_SHADER_STORAGE_BUFFER_);
        } else
            fail("could not map the command buffer");

        checkErrors("reading the buffers back");
    }

    // Clean up and give the game its GL state back
    saved.restore();

    if (vertexArray) gl.DeleteVertexArrays(1, &vertexArray);
    if (framebuffer) gl.DeleteFramebuffers(1, &framebuffer);
    if (textures[0]) gl.DeleteTextures(2, textures);
    if (buffers[0]) gl.DeleteBuffers(4, buffers);
    if (cullProgram) gl.DeleteProgram(cullProgram);
    if (finalizeProgram) gl.DeleteProgram(finalizeProgram);
    if (drawProgram) gl.DeleteProgram(drawProgram);
    checkErrors("cleanup");

    if (!failed) {
        // What the CPU expects
        unsigned expectedCounters[2] = { 0, 0 };
        for (unsigned i = 0; i < SPRITE_COUNT; i++) {
            if (i % 3 != 0)
                expectedCounters[i % 2]++;
        }

        note("counters: " + std::to_string(counterResult[0]) + ", " + std::to_string(counterResult[1]) +
             " (expected " + std::to_string(expectedCounters[0]) + ", " + std::to_string(expectedCounters[1]) + ")");

        for (unsigned layer = 0; layer < LAYER_COUNT; layer++) {
            std::string line = "command " + std::to_string(layer) + ":";
            for (unsigned field = 0; field < 5; field++)
                line += " " + std::to_string(commandResult[layer * 5 + field]);
            note(line + " (count, instances, first index, base vertex, base instance)");
        }

        if (counterResult[0] == expectedCounters[0] && counterResult[1] == expectedCounters[1])
            ok("atomic compaction counted the right number of sprites per layer");
        else
            fail("atomic compaction counted the wrong number of sprites");

        unsigned red = 0, blue = 0, other = 0;
        for (size_t i = 0; i < pixels.size(); i += 4) {
            unsigned r = pixels[i], g = pixels[i + 1], b = pixels[i + 2], a = pixels[i + 3];
            if (r > 200 && g < 50 && b < 50 && a > 200)
                red++;
            else if (b > 200 && r < 50 && g < 50 && a > 200)
                blue++;
            else if (r || g || b || a)
                other++;
        }

        unsigned expectedRed  = expectedCounters[0] * 4;
        unsigned expectedBlue = expectedCounters[1] * 4;

        note("pixels: " + std::to_string(red) + " red, " + std::to_string(blue) + " blue, " +
             std::to_string(other) + " other (expected " + std::to_string(expectedRed) + " red, " +
             std::to_string(expectedBlue) + " blue, 0 other)");

        if (red == expectedRed && blue == expectedBlue && other == 0)
            ok("indirect draws read the compute-written index buffer and sprite texture correctly");
        else
            fail("the image does not match, a barrier, the texture fetch or the indirect draw is not working");
    }

    passed = !failed;
    note(passed ? "RESULT: PASS" : "RESULT: FAIL");
    return report;
}
