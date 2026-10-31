# Android (OpenGL ES) port

The renderer uses desktop OpenGL 4.3+ (compute shaders, SSBOs, multi draw indirect) and
Windows-only function loading. Android only has OpenGL ES, so it is ported in phases.

| Phase | What | State |
|-------|------|-------|
| 1 | Builds for android64 (only `src/android/` is compiled, see CMakeLists.txt) | done |
| 2 | Find out what the device's GL context supports (`gl_probe.txt`) | done |
| 2b | Smoke test of the ES design on the device (`gles_smoketest.txt`) | done, PASS |
| M1 | Whole mod compiles for android64: ES function loader (`src/es.hpp`), shaders expanded without glslang, no hotkeys/profiler. It falls back to the vanilla renderer at runtime (the vertex shader still uses an SSBO) | in progress |
| 3 | Shaders OpenGL ES clean (no implicit int/uint/float conversions) and raw sprites in an RGBA32F texture (imageStore in compute, texelFetch in the vertex shader) | done: the renderer runs on the phone |
| 4 | Culling in 3 passes (cull, per-layer scan, copy): one draw call per layer and blend mode instead of ~10k per frame, blending decided on the GPU every frame | built, first run on device |
| 5 | Fit the SSBO usage into the device limits | todo |
| 6 | Debug on a real device | todo |

Phase 4 and 5 depend on the numbers the probe reports.

## Build
Push this repo to GitHub, run the "Build Geode Mod" workflow (Actions tab), download the
`.geode` file from the artifacts and put it in the Geode mods folder on the phone.

## What the probe found (Mali-G78, OpenGL ES 3.2)
- Vertex shaders can't use SSBOs (0 blocks, compute has 35). Per-sprite data has to go through a texture
  written by compute (imageStore) and read with texelFetch.
- No GL_EXT_multi_draw_indirect. Each layer needs one contiguous index list and a single glDrawElementsIndirect.
- Compute limits are generous (512 invocations per group, 268 MB SSBOs), buffer_storage is available.

## Smoke test
`src/android/smoketest.cpp` runs the design above on a tiny scene (256 sprites, 2 layers) and checks the pixels.

## Building with all the errors at once
`BISMUTH_KEEP_GOING` (CMake option, on) makes a compile error not stop the build, see `tools/keepgoing.sh`.
The link step then fails, but the log lists the errors of every file. Turn it off when everything compiles.

## Diagnostics
On Android `Renderer::logDiagnostics` writes two `[diag]` lines to the Geode log every ~3 seconds while a level is
running: camera and matrix, how many group combinations are hidden/visible/fading, how many color channels have
alpha, how many indices the culling shader kept, and GL errors. Remove it when the port is done.

## Debug tools
Everything that is only for debugging is behind the "Enable debug tools" setting (`bismuth::debugTools`, read when a
level starts): all logging (`blog::`), the `[diag]` counters (compiled out of the culling shader when off), the
debug text and the profiler timers. With it off the mod never logs. Note that this setting also turns off the
CPU side optimizations (existing behavior, see Renderer::useOptimizations), so measure FPS with it off.
