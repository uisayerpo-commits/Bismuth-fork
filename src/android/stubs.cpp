/*
    The profiler (src/profiler.cpp) hooks a lot of engine functions and is not built on
    Android. These are the functions the rest of the code calls, doing nothing.
*/

#include <profiler.hpp>

bool profiler::isTakingSnapshot = false;

void profiler::functionPush(const char*) {}

void profiler::functionPop() {}

void profiler::addTimer(GPUTimer*) {}

void profiler::removeTimer(GPUTimer*) {}
