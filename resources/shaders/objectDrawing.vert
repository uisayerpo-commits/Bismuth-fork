#extension GL_ARB_shading_language_include:require

#include "shared.h"

//// VERTEX ATTRIBUTES ////
layout (location = 0) in vec2 a_positionOffset;
layout (location = 1) in vec2 a_texCoord;
layout (location = 2) in int  a_spriteIndex;

//// VARIABLES TO BE TRANSFERED TO THE FRAGMENT SHADER ////
out vec2 t_texCoord;
out vec4 t_color;

// Raw sprites are written by the culling compute shader (see headers/rawSpriteBuffer.h)
#ifdef RAW_SPRITES_IN_TEXTURE
uniform highp sampler2D u_rawSpriteTexture;

vec4 fetchRawSpriteTexel(uint texel) {
    return texelFetch(u_rawSpriteTexture, ivec2(int(texel % RAW_SPRITE_TEXTURE_WIDTH), int(texel / RAW_SPRITE_TEXTURE_WIDTH)), 0);
}
#else
#define RAW_SPRITE (u_rawSprites[a_spriteIndex])
#endif

//// MAIN FUNCTION ////
void main() {
#ifdef RAW_SPRITES_IN_TEXTURE
    uint first = uint(a_spriteIndex) * RAW_SPRITE_TEXELS;
    vec4 data0 = fetchRawSpriteTexel(first);
    vec4 data1 = fetchRawSpriteTexel(first + 1u);
    vec4 data2 = fetchRawSpriteTexel(first + 2u);

    vec4 spriteColor    = data0;
    vec2 spritePosition = data1.xy;
    mat2 spriteTransform = mat2(data1.zw, data2.xy);
#else
    vec4 spriteColor    = RAW_SPRITE.color;
    vec2 spritePosition = RAW_SPRITE.position;
    mat2 spriteTransform = RAW_SPRITE.localTransform;
#endif

    vec2 vertexPos = spritePosition + spriteTransform * a_positionOffset;

    gl_Position = u_mvp * vec4(vertexPos, 0.0, 1.0);

    t_color    = spriteColor;
    t_texCoord = a_texCoord;
}
