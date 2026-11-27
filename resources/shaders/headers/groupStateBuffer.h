#define GROUP_IDS_PER_OBJECT 10

/*
    The current state of a group combination.
    This info can change every frame.
*/
struct GroupCombinationState {
    mat2 positionalTransform;
    mat2 localTransform;
    vec2 offset;
    float opacity;
    // Bit N of the first mask, N - 32 of the second, is set when the group combination contains the
    // target group of area effect slot N (area tint and area fade, see tintBuffer.h)
    uint tintMask;
    uint tintMask2;
    // Where the pulses of the groups of this combination are in u_pulseWords (see colorChannelBuffer.h):
    // the first entry in the lower 20 bits, the number of entries above that. 0 when none.
    uint pulseIndex;
};

#ifndef NO_STORAGE_BUFFERS
STORAGE_BUFFER(GROUP_STATE_BUFFER_BINDING) GroupStateBuffer {
    GroupCombinationState u_groupCombinationStates[CPP_ONLY(0)];
};
#endif