#pragma once

#include <functional>
#include <string>

/*
    Runs a small version of the OpenGL ES renderer design on the current GL context
    and returns a report. It needs to be called on the main (GL) thread.

    The design that is tested:
      1. A compute shader culls sprites and compacts the visible ones into one index
         list per layer (atomic counters). It also writes the sprite data into a texture
         with imageStore, because vertex shaders can't read SSBOs on Mali (0 blocks).
      2. A second compute shader turns the counters into draw commands.
      3. One glDrawElementsIndirect per layer draws from the compacted indices, the
         vertex shader reads the sprite data with texelFetch.
      4. The result is read back and compared with what the CPU expects.

    `progress` is called with the whole report so far after every step. If the driver
    crashes the game, the file it writes shows how far the test got.
*/
std::string runGlesSmokeTest(bool& passed, const std::function<void(const std::string&)>& progress);
