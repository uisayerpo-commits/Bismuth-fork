#pragma once

#include "common.hpp"

class BProfiler {
public:
    class Timer;

public:
    // The name has to be a string literal (or live as long as the timer does)
    static Timer start(const char* name);

    static bool isUsingAverages();

    static void useAverages(bool cond);

    static std::string toString();

    static void frameEnd();

private:
    static void end(const char* name, u64 time);

public:
    struct Timer {
    private:
        const char* name;
        u64 lastTime;
        bool active;

    public:
        // Reads the clock only when the debug tools are on
        inline Timer(const char* name)
            : name(name), active(bismuth::debugTools), lastTime(bismuth::debugTools ? getTime() : 0) {}

        inline void end() {
            if (active)
                BProfiler::end(name, getTime() - lastTime);
        }
    };
};