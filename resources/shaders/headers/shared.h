/*
    This header file is shared between the shaders and
    the actual c++ source code of the mod.

    The 'GLSL' macro variable is only present in the
    shader code and can be used to disinguish
*/

#ifndef SHADER_SHARED_H
#define SHADER_SHARED_H

#ifndef GLSL

#include <common.hpp>
// Set the correct std430 alignments
#define vec2 alignas(8)  glm::vec2
#define vec3 alignas(16) glm::vec3
#define vec4 alignas(16) glm::vec4
#define mat2 alignas(8)  glm::mat2
#define mat3 alignas(16) glm::mat3
#define mat4 alignas(16) glm::mat4
#define bool alignas(4) bool
#define uint unsigned int

struct alignas(4) RGBA { u8 r, g, b, a; };

// In GLSL, these defines are set by the renderer
#define GROUP_ID_LIMIT     0
#define TOTAL_OBJECT_COUNT 0

#define GLSL_ONLY(D)
#define CPP_ONLY(D) D

#define UNIFORM_BUFFER(_BINDING) struct
#define STORAGE_BUFFER(_BINDING) struct

#else // GLSL

#define SPRITE_SHEET_GAME_1   0
#define SPRITE_SHEET_GAME_2   1
#define SPRITE_SHEET_TEXT     2
#define SPRITE_SHEET_FIRE     3
#define SPRITE_SHEET_SPECIAL  4
#define SPRITE_SHEET_GLOW     5
#define SPRITE_SHEET_PIXEL    6
#define SPRITE_SHEET_UNK      7
#define SPRITE_SHEET_PARTICLE 8

#define RGBA uint
#define RGBA_TO_VEC4(V) vec4( \
    float((V) & 0xffu) / 255.0, \
    float(((V) >> 8) & 0xffu) / 255.0, \
    float(((V) >> 16) & 0xffu) / 255.0, \
    float(((V) >> 24) & 0xffu) / 255.0 \
)
#define BitmapThing uint

const float PI = 3.1415926535897932384626433832795;

vec2 rotatePointAroundOrigin(vec2 point, float angleInRadians) {
    float rotSin = sin(angleInRadians);
    float rotCos = cos(angleInRadians);

    return vec2(
        point.x * rotCos - point.y * rotSin,
        point.x * rotSin + point.y * rotCos
    );
}

mat2 rotationMatrix(float angleInRadians) {
    float s = sin(angleInRadians);
    float c = cos(angleInRadians);

    return mat2(
         c, s,
        -s, c
    );
}

mat2 scaleMatrix(float scale) {
    return mat2(
        scale, 0.0,
        0.0, scale
    );
}

#define BITMAP_GET(B, I) ( ( (B)[(I) >> 5] >> ((I) & 0x1fu) ) & 1u )

#define GET_LOW16(V)  ( (V)       & 0xffffu )
#define GET_HIGH16(V) ( (V >> 16) & 0xffffu )

#define GLSL_ONLY(D) D
#define CPP_ONLY(D)

#define UNIFORM_BUFFER(_BINDING) layout (std140, binding = _BINDING) uniform
#define STORAGE_BUFFER(_BINDING) layout (std430, binding = _BINDING) buffer

#endif // GLSL

#include "hsv.h"

/*
    Number of work groups (see workGroupBuffer.h) the object culling
    compute shader processes per GPU thread group. Every invocation
    handles exactly one work group, the invocations don't depend on
    each other. Running them 1 per thread group would leave all but
    one lane of every wave/warp idle.
*/
#define CULLING_LOCAL_SIZE 64

/*
    OpenGL ES GPUs like the Mali ones can't read shader storage buffers in the vertex shader
    (GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS is 0). When RAW_SPRITES_IN_TEXTURE is defined (Android)
    the raw sprites are written by the culling compute shader into an RGBA32F texture with
    imageStore, and read by the vertex shader with texelFetch. Every raw sprite is
    RAW_SPRITE_TEXELS texels, texel number N is at (N % WIDTH, N / WIDTH):
        texel 0: color
        texel 1: position (xy), local transform first column (zw)
        texel 2: local transform second column (xy), unused (zw)
*/
// The texture unit the vertex shader reads the raw sprites from, nothing else in the game uses it
#define RAW_SPRITE_TEXTURE_UNIT  15
#define RAW_SPRITE_TEXTURE_WIDTH 4096u
#define RAW_SPRITE_TEXELS        3u

////////////////////////////////////////////////////
//// UNIFORM BUFFERS AND SHADER STORAGE BUFFERS ////
////////////////////////////////////////////////////

#define COLOR_CHANNEL_BUFFER_BINDING     0
#define GROUP_STATE_BUFFER_BINDING       1
#define OBJECT_INFO_BUFFER_BINDING       2
#define GAME_STATE_BUFFER_BINDING        3
#define RAW_SPRITE_BUFFER_BINDING        4
#define INPUT_SPRITE_BUFFER_BINDING      5
#define WORK_GROUP_BUFFER_BINDING        6
#define INPUT_INDEX_BUFFER_BINDING       7
#define CULLED_INDEX_BUFFER_BINDING      8
#define DRAW_COMMAND_BUFFER_BINDING      9
#define SPRITE_REMAP_BUFFER_BINDING      10
#define PRE_CULLED_SPRITE_BUFFER_BINDING 11
#define DIAGNOSTICS_BUFFER_BINDING       12
#define SPRITE_STATE_BUFFER_BINDING      13
#define WORK_GROUP_OUTPUT_BUFFER_BINDING 14
#define LAYER_INFO_BUFFER_BINDING        15
// 16 counters, then one counter per color channel for the sprites that were drawn,
// then one per color channel for the sprites that ended up with alpha 0 (1101 color channels)
#define DIAGNOSTICS_SCALAR_COUNT         16
#define DIAGNOSTICS_DRAWN_CHANNEL_BASE   16
#define DIAGNOSTICS_HIDDEN_CHANNEL_BASE  1117
#define DIAGNOSTICS_COUNTER_COUNT        2218

#include "colorChannelBuffer.h"
#include "groupStateBuffer.h"
#include "objectInfoBuffer.h"
#include "gameStateBuffer.h"
#include "rawSpriteBuffer.h"
#include "inputSpriteBuffer.h"
#include "workGroupBuffer.h"

/*
    This is the structure of the draw command to
    be executed by glMultiDrawElementsIndirect
*/
struct DrawCommand {
    uint count;
    uint instanceCount;
    uint firstIndex;
    int  baseVertex;
    uint baseInstance;
};

#ifndef GLSL
// Remove the changed alignments
#undef vec2
#undef vec3
#undef vec4
#undef mat2
#undef mat3
#undef mat4
#undef bool
#undef uint
#undef HSV
#undef UNIFORM_BUFFER
#undef STORAGE_BUFFER
#endif

#endif // SHADER_SHARED_H