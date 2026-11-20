/*
    Area tint trigger instances that are active this frame (see Renderer::captureTintInstance).

    Every instance takes 4 vec4s in u_tintData:
        [0] center.x, center.y, length, length variance
        [1] offset variance, offset Y variance, mod front, mod back
        [2] deadzone, tint amount, tint color r, tint color g
        [3] tint color b, slot, mode (0 radial, 1 x, 2 y), flags (1 reversed, 2 tints base, 4 tints detail)
    'slot' (0 to 63) is the bit of GroupCombinationState::tintMask and tintMask2 for the instance's
    target group. Tint and fade share the slots, a group has the same slot in both.

    Area fade instances are in u_fadeData, also 4 vec4s each:
        [0] center.x, center.y, length, length variance
        [1] offset variance, offset Y variance, mod front, mod back
        [2] deadzone, from opacity, to opacity, slot
        [3] mode (0 radial, 1 x, 2 y), flags (1 reversed), 0, 0

    u_tintVariance is the game's table of 2000 random numbers (GJBaseGameLayer::m_varianceValues),
    4 per vec4.
*/
#define TINT_MAX_INSTANCES    32
#define TINT_DATA_VEC4_COUNT  (TINT_MAX_INSTANCES * 4)
#define FADE_MAX_INSTANCES    48
#define FADE_DATA_VEC4_COUNT  (FADE_MAX_INSTANCES * 4)
#define TINT_VARIANCE_COUNT   2000
#define TINT_VARIANCE_VEC4S   (TINT_VARIANCE_COUNT / 4)

#ifndef NO_STORAGE_BUFFERS
UNIFORM_BUFFER(TINT_BUFFER_BINDING) TintBuffer {
    uint u_tintCount;
    uint u_fadeCount;
    // The HSV (packed like ObjectInfo::baseHSV) that turns the color of an object into the color of the
    // "Lighter" channel (1012). 0 when the game's values couldn't be read, see Renderer::setupLighterColor
    uint u_lighterHSV;
    vec4 u_tintData[TINT_DATA_VEC4_COUNT];
    vec4 u_fadeData[FADE_DATA_VEC4_COUNT];
    vec4 u_tintVariance[TINT_VARIANCE_VEC4S];
};
#endif
