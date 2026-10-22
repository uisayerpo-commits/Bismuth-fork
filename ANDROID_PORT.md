# Android (OpenGL ES) port

The renderer uses desktop OpenGL 4.3+ (compute shaders, SSBOs, multi draw indirect) and
Windows-only function loading. Android only has OpenGL ES, so it is ported in phases.

| Phase | What | State |
|-------|------|-------|
| 1 | Builds for android64 (only `src/android/` is compiled, see CMakeLists.txt) | in progress |
| 2 | Find out what the device's GL context supports (probe popup on the main menu) | in progress |
| 3 | Port the shaders to `#version 310 es` (precision qualifiers, no glslang) | todo |
| 4 | Culling output: one draw call per layer instead of multi draw indirect | todo |
| 5 | Fit the SSBO usage into the device limits | todo |
| 6 | Debug on a real device | todo |

Phase 4 and 5 depend on the numbers the probe reports.

## Build
Push this repo to GitHub, run the "Build Geode Mod" workflow (Actions tab), download the
`.geode` file from the artifacts and put it in the Geode mods folder on the phone.
