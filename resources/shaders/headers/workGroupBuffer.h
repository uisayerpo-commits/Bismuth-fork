/*
    This contains the info for a single invocation of the
    object culling compute shader. It tells the shader
    which input sprites to process. The input sprites
    can be found inside the input sprite buffer.
*/
struct WorkGroup {
    uint inputSpriteStart;
    uint inputSpriteEnd;
    // This tells the compute shader where to output the indicies in the culled index buffer
    uint culledIndexBufferOutputIndex;
};

#define MAX_WORK_GROUP_COUNT 10000

#ifndef NO_STORAGE_BUFFERS
STORAGE_BUFFER(WORK_GROUP_BUFFER_BINDING) WorkGroupBuffer {
    WorkGroup u_workGroups[CPP_ONLY(0)];
};
#endif

/*
    The culling is done in 3 passes (see objectCulling.comp):
      1. every work group culls its sprites, writes the raw sprites, and counts how many indices
         it keeps for the normal pass and for the additive (blending) pass
      2. every layer adds up the counts of its work groups in order, giving each work group the place
         in the index buffer where its indices go, and writes the 2 draw commands of the layer
      3. every work group copies the indices of its sprites there
    That makes the indices of a layer one contiguous list (per pass) in the right order, so a layer is
    drawn with a single draw call instead of one per work group.
*/

// One per work group
struct WorkGroupOutput {
    // Written by pass 1
    uint normalIndexCount;
    uint blendIndexCount;
    // Written by pass 2, where in the culled index buffer the indices of the work group start
    uint normalOffset;
    uint blendOffset;
};

#ifndef NO_STORAGE_BUFFERS
STORAGE_BUFFER(WORK_GROUP_OUTPUT_BUFFER_BINDING) WorkGroupOutputBuffer {
    WorkGroupOutput u_workGroupOutputs[CPP_ONLY(0)];
};
#endif

// One per layer (z layer and sprite sheet), never changes
struct LayerInfo {
    uint firstWorkGroup;
    uint workGroupCount;
    // The part of the culled index buffer that belongs to this layer. Normal sprites are written
    // from the start of it, blending sprites to its end. Both together never need more than indexTotal.
    uint indexBase;
    uint indexTotal;
};

#ifndef NO_STORAGE_BUFFERS
STORAGE_BUFFER(LAYER_INFO_BUFFER_BINDING) LayerInfoBuffer {
    LayerInfo u_layers[CPP_ONLY(0)];
};
#endif

// One per input sprite, written by pass 1: 0 = not drawn, 1 = normal pass, 2 = additive pass
#ifndef NO_STORAGE_BUFFERS
STORAGE_BUFFER(SPRITE_STATE_BUFFER_BINDING) SpriteStateBuffer {
    uint u_spriteStates[CPP_ONLY(0)];
};
#endif
