# Unreleased
- Falls back to the vanilla renderer (instead of crashing) when the GPU is older than OpenGL 4.3, a shader fails to compile or the GPU runs out of memory
- Added the missing memory barrier between the culling compute shader and the draw calls
- The culling compute shader now uses 64 invocations per thread group instead of 1
- Buffers are no longer left mapped while the GPU uses them
- Fixed crashes from move/rotate triggers that target groups without objects
- Fixed rotate triggers without a center only rotating the first group combination
- Fixed GPU memory, shader and node leaks every time a level is loaded
- Group states are only uploaded when they change
- The debug text is only updated when it changes
- F8, F9 and P (debug hotkeys) only work when "Enable debug tools" is on
- Faster level loading

# 0.1.0-alpha
- Added rotate trigger support
- Added follow trigger support
- Added shader sprite optimization
- Added seperate batch node optimization
- Added index culling optimization
- Fixed Intel (and AMD?) GPU crashes

# 0.0.1-alpha.1
- Added renderer
- Added color trigger support
- Added alpha trigger support
- Added move trigger support
- Added toggle trigger support
- Added object HSV
- Objects now rotate
- Pulsating objects now pulse to audio scale