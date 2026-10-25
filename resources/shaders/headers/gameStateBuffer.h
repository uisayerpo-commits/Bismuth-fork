#define GAME_STATE_IS_PLAYER_DEAD (1u << 0)
// Set on the frames where the culling shader counts what happens to the sprites (see DIAGNOSTICS_BUFFER_BINDING)
#define GAME_STATE_COLLECT_DIAGNOSTICS (1u << 1)

/*
    This is the uniform buffer used to store
    game state uniforms.
*/
UNIFORM_BUFFER(GAME_STATE_BUFFER_BINDING) GameStateBuffer {
    mat4  u_mvp;
    float u_timer;
    float u_audioScale;
    vec2  u_cameraViewSize;

    vec2  u_cameraPosition;
    vec2  u_cameraRight;
    vec2  u_cameraUp;
    vec2  u_cameraSize;

    // Used for invisible block calculation
    vec2  u_winSize;
    float u_screenRight;
    float u_cameraUnzoomedX;
    vec3  u_specialLightBGColor;

    uint  u_gameStateFlags;
};