/*
    Area tint trigger instances that are active this frame (see Renderer::captureTintInstance).

    Every instance takes 4 vec4s in u_tintData:
        [0] center.x, center.y, length, length variance
        [1] offset variance, offset Y variance, mod front, mod back
        [2] deadzone, tint amount, tint color r, tint color g
        [3] tint color b, slot, mode (0 radial, 1 x, 2 y), flags (1 reversed, 2 tints base, 4 tints detail)
    'slot' is the bit of GroupCombinationState::tintMask for the instance's target group.

    u_tintVariance is the game's table of 2000 random numbers (GJBaseGameLayer::m_varianceValues),
    4 per vec4.
*/
#define TINT_MAX_INSTANCES    32
#define TINT_DATA_VEC4_COUNT  (TINT_MAX_INSTANCES * 4)
#define TINT_VARIANCE_COUNT   2000
#define TINT_VARIANCE_VEC4S   (TINT_VARIANCE_COUNT / 4)

#ifndef NO_STORAGE_BUFFERS
UNIFORM_BUFFER(TINT_BUFFER_BINDING) TintBuffer {
    uint u_tintCount;
    vec4 u_tintData[TINT_DATA_VEC4_COUNT];
    vec4 u_tintVariance[TINT_VARIANCE_VEC4S];
};
#endif
