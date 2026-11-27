#define COLOR_CHANNEL_COUNT 1101

#define COLOR_CHANNEL_BG      1000
#define COLOR_CHANNEL_G1      1001
#define COLOR_CHANNEL_LINE    1002
#define COLOR_CHANNEL_3DL     1003
#define COLOR_CHANNEL_OBJ     1004
#define COLOR_CHANNEL_P1      1005
#define COLOR_CHANNEL_P2      1006
#define COLOR_CHANNEL_LBG     1007
#define COLOR_CHANNEL_G2      1009
#define COLOR_CHANNEL_BLACK   1010
#define COLOR_CHANNEL_WHITE   1011
#define COLOR_CHANNEL_LIGHTER 1012
#define COLOR_CHANNEL_MG      1013
#define COLOR_CHANNEL_MG2     1014

/*
    Glow sprites (the ones from the glow sheet) are always drawn additively by the game, whatever
    the blending of their color channel is. They use this channel for the blending decision. The
    channel is never used for a color, and the CPU keeps its blending bit set.
*/
#define COLOR_CHANNEL_ALWAYS_BLENDING 1100

// One bit per color channel, 1 if the channel is blending (additive) right now
#define COLOR_CHANNEL_BLENDING_WORDS 35

/*
    Pulse triggers that target a group. The entries are behind the color channels in this buffer
    (the shaders can't take another storage buffer), 4 words each, the CPU writes them at the byte
    offset sizeof(ColorChannelBuffer). A group combination points to its entries with pulseIndex.

        word 0: bits 0-15 the strength of the pulse (0 to 65535), bits 16-25 the channel to copy the
                color from (0 for none), bits 26-31 the PULSE_FLAG_ bits
        word 1: the color (r in the lowest byte) of a color pulse, the hue of an HSV pulse as a float
        word 2: the saturation of an HSV pulse as a float
        word 3: the brightness of an HSV pulse as a float
*/
#define PULSE_ENTRY_WORDS      4
#define PULSE_ENTRY_CAPACITY   8192

#define PULSE_FLAG_BASE_ONLY   (1u << 26)
#define PULSE_FLAG_DETAIL_ONLY (1u << 27)
#define PULSE_FLAG_HSV         (1u << 28)
#define PULSE_FLAG_DYNAMIC_HSV (1u << 29)
#define PULSE_FLAG_SAT_ADD     (1u << 30)
#define PULSE_FLAG_VAL_ADD     (1u << 31)

#ifndef NO_STORAGE_BUFFERS
STORAGE_BUFFER(COLOR_CHANNEL_BUFFER_BINDING) ColorChannelBuffer {
    RGBA u_channelColors[COLOR_CHANNEL_COUNT];
    uint u_channelBlending[COLOR_CHANNEL_BLENDING_WORDS];
    uint u_pulseWords[CPP_ONLY(0)];
};
#endif