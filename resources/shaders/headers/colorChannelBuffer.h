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

// One bit per color channel, 1 if the channel is blending (additive) right now
#define COLOR_CHANNEL_BLENDING_WORDS 35

#ifndef NO_STORAGE_BUFFERS
STORAGE_BUFFER(COLOR_CHANNEL_BUFFER_BINDING) ColorChannelBuffer {
    RGBA u_channelColors[COLOR_CHANNEL_COUNT];
    uint u_channelBlending[COLOR_CHANNEL_BLENDING_WORDS];
};
#endif