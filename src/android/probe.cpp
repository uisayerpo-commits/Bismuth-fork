/*
    Android port, phase 1 + 2.

    On Android only this file is built (see CMakeLists.txt). It doesn't render
    anything. When the main menu opens for the first time it asks the GL context
    what it can do and shows the answer in a popup (and in the Geode log).

    Those numbers decide how the OpenGL ES renderer has to be designed:
    compute shaders, shader storage buffers (SSBOs) and indirect drawing are
    all optional or limited in OpenGL ES.
*/

#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>

#include "smoketest.hpp"

#include <dlfcn.h>

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>
#include <string>
#include <thread>

using namespace geode::prelude;

/*
    The GL functions are looked up at runtime instead of including GLES3/gl31.h.
    Geode's own Android GL header renames some GL functions with macros
    (glBindVertexArray -> glBindVertexArrayOES...) and that clashes with the NDK header.
    All the tokens below are written out for the same reason.
*/
using GLenumT = unsigned int;
using GLintT  = int;
using GLuintT = unsigned int;

constexpr GLenumT BIS_NO_ERROR      = 0;
constexpr GLenumT BIS_VENDOR        = 0x1F00;
constexpr GLenumT BIS_RENDERER      = 0x1F01;
constexpr GLenumT BIS_VERSION       = 0x1F02;
constexpr GLenumT BIS_EXTENSIONS    = 0x1F03;
constexpr GLenumT BIS_GLSL_VERSION  = 0x8B8C;
constexpr GLenumT BIS_NUM_EXTENSIONS = 0x821D;

// Tokens from OpenGL ES 3.1
constexpr GLenumT BIS_MAX_VERTEX_SSBO         = 0x90D6;
constexpr GLenumT BIS_MAX_FRAGMENT_SSBO       = 0x90DA;
constexpr GLenumT BIS_MAX_COMPUTE_SSBO        = 0x90DB;
constexpr GLenumT BIS_MAX_COMBINED_SSBO       = 0x90DC;
constexpr GLenumT BIS_MAX_SSBO_SIZE           = 0x90DE;
constexpr GLenumT BIS_MAX_COMPUTE_INVOCATIONS = 0x90EB;
constexpr GLenumT BIS_MAX_COMPUTE_SHARED_MEM  = 0x8262;
constexpr GLenumT BIS_MAX_COMPUTE_GROUP_COUNT = 0x91BE;
constexpr GLenumT BIS_MAX_COMPUTE_GROUP_SIZE  = 0x91BF;

static const unsigned char* (*pGetString)(GLenumT) = nullptr;
static const unsigned char* (*pGetStringi)(GLenumT, GLuintT) = nullptr;
static void (*pGetIntegerv)(GLenumT, GLintT*) = nullptr;
static void (*pGetIntegeri_v)(GLenumT, GLuintT, GLintT*) = nullptr;
static GLenumT (*pGetError)() = nullptr;

static void* findGLSymbol(const char* name) {
    void* symbol = dlsym(RTLD_DEFAULT, name);
    if (symbol)
        return symbol;

    for (const char* library : { "libGLESv3.so", "libGLESv2.so" }) {
        if (void* handle = dlopen(library, RTLD_NOW)) {
            symbol = dlsym(handle, name);
            if (symbol)
                return symbol;
        }
    }
    return nullptr;
}

static bool loadGLFunctions() {
    pGetString     = reinterpret_cast<decltype(pGetString)>(findGLSymbol("glGetString"));
    pGetStringi    = reinterpret_cast<decltype(pGetStringi)>(findGLSymbol("glGetStringi"));
    pGetIntegerv   = reinterpret_cast<decltype(pGetIntegerv)>(findGLSymbol("glGetIntegerv"));
    pGetIntegeri_v = reinterpret_cast<decltype(pGetIntegeri_v)>(findGLSymbol("glGetIntegeri_v"));
    pGetError      = reinterpret_cast<decltype(pGetError)>(findGLSymbol("glGetError"));

    // glGetStringi and glGetIntegeri_v only exist in OpenGL ES 3.0+, the rest in every version
    return pGetString && pGetIntegerv && pGetError;
}

static int getInt(GLenumT name) {
    GLintT value = -1;
    while (pGetError() != BIS_NO_ERROR) {}
    pGetIntegerv(name, &value);
    if (pGetError() != BIS_NO_ERROR)
        return -1;
    return value;
}

static int getIndexedInt(GLenumT name, GLuintT index) {
    if (!pGetIntegeri_v)
        return -1;

    GLintT value = -1;
    while (pGetError() != BIS_NO_ERROR) {}
    pGetIntegeri_v(name, index, &value);
    if (pGetError() != BIS_NO_ERROR)
        return -1;
    return value;
}

static std::string buildReport() {
    std::ostringstream out;

    if (!loadGLFunctions()) {
        out << "Could not find the OpenGL ES functions in the process.\n";
        return out.str();
    }

    const char* version  = (const char*)pGetString(BIS_VERSION);
    const char* renderer = (const char*)pGetString(BIS_RENDERER);
    const char* vendor   = (const char*)pGetString(BIS_VENDOR);
    const char* glsl     = (const char*)pGetString(BIS_GLSL_VERSION);

    out << "Version: "  << (version  ? version  : "?") << "\n";
    out << "Renderer: " << (renderer ? renderer : "?") << "\n";
    out << "Vendor: "   << (vendor   ? vendor   : "?") << "\n";
    out << "GLSL: "     << (glsl     ? glsl     : "?") << "\n";

    int major = 0, minor = 0;
    if (version) {
        // "OpenGL ES 3.2 v1.r32p1-..." -> 3, 2
        const char* digits = version;
        while (*digits && !(*digits >= '0' && *digits <= '9'))
            digits++;
        sscanf(digits, "%d.%d", &major, &minor);
    }

    bool hasEs31 = major > 3 || (major == 3 && minor >= 1);
    if (!hasEs31) {
        out << "\nNo OpenGL ES 3.1: there are no compute shaders or SSBOs, the GPU culling design can't work here.\n";
        return out.str();
    }

    out << "\nSSBO blocks (vertex/fragment/compute/combined): "
        << getInt(BIS_MAX_VERTEX_SSBO) << " / "
        << getInt(BIS_MAX_FRAGMENT_SSBO) << " / "
        << getInt(BIS_MAX_COMPUTE_SSBO) << " / "
        << getInt(BIS_MAX_COMBINED_SSBO) << "\n";
    out << "Max SSBO size: " << getInt(BIS_MAX_SSBO_SIZE) << "\n";
    out << "Compute invocations per group: " << getInt(BIS_MAX_COMPUTE_INVOCATIONS) << "\n";
    out << "Compute group size: "
        << getIndexedInt(BIS_MAX_COMPUTE_GROUP_SIZE, 0) << " x "
        << getIndexedInt(BIS_MAX_COMPUTE_GROUP_SIZE, 1) << " x "
        << getIndexedInt(BIS_MAX_COMPUTE_GROUP_SIZE, 2) << "\n";
    out << "Compute group count: "
        << getIndexedInt(BIS_MAX_COMPUTE_GROUP_COUNT, 0) << " x "
        << getIndexedInt(BIS_MAX_COMPUTE_GROUP_COUNT, 1) << " x "
        << getIndexedInt(BIS_MAX_COMPUTE_GROUP_COUNT, 2) << "\n";
    out << "Compute shared memory: " << getInt(BIS_MAX_COMPUTE_SHARED_MEM) << "\n";

    std::set<std::string> extensions;
    int extensionCount = getInt(BIS_NUM_EXTENSIONS);
    for (int i = 0; pGetStringi && i < extensionCount; i++) {
        const char* ext = (const char*)pGetStringi(BIS_EXTENSIONS, i);
        if (ext)
            extensions.insert(ext);
    }

    out << "\nExtensions: " << extensions.size() << " total\n";
    const char* interesting[] = {
        "GL_EXT_multi_draw_indirect",
        "GL_EXT_buffer_storage",
        "GL_EXT_disjoint_timer_query",
        "GL_KHR_debug",
        "GL_EXT_draw_elements_base_vertex",
        "GL_EXT_shader_non_constant_global_initializers",
    };
    for (auto name : interesting)
        out << (extensions.contains(name) ? "yes  " : "no   ") << name << "\n";

    // The full list goes to the log only, it's too long for a popup
    std::ostringstream all;
    for (auto& ext : extensions)
        all << ext << " ";
    log::info("GL extensions: {}", all.str());

    return out.str();
}

// The files end up in the mod's save folder, so they can be read without logcat
static void writeTextFile(const char* name, const std::string& text) {
    std::error_code error;
    auto directory = Mod::get()->getSaveDir();
    std::filesystem::create_directories(directory, error);

    std::ofstream file(directory / name, std::ios::trunc);
    file << text;
}

// Proves the mod was loaded, even if nothing shows up on screen
$on_mod(Loaded) {
    writeTextFile("loaded.txt", "Bismuth Android probe loaded\n");
    log::info("Bismuth Android probe loaded");
}

// Runs on the main thread, where the GL context is current
static void runProbe() {
    static bool done = false;
    if (done)
        return;
    done = true;

    std::string report = buildReport();
    writeTextFile("gl_probe.txt", report);
    log::info("Bismuth GL probe:\n{}", report);

    // Tests the OpenGL ES version of the renderer design on this GPU. The report is
    // rewritten after every step, so if the driver crashes the game the file shows where.
    bool passed = false;
    std::string smokeTest = runGlesSmokeTest(passed, [](const std::string& progress) {
        writeTextFile("gles_smoketest.txt", progress);
    });
    writeTextFile("gles_smoketest.txt", smokeTest);
    log::info("Bismuth GLES smoke test:\n{}", smokeTest);

    // Short on purpose, the full text is in the files. The popup parses '<' and '>' as color tags.
    std::string summary = std::string("Smoke test: ") + (passed ? "PASS" : "FAIL") +
        "\n\nDetails are in gles_smoketest.txt and gl_probe.txt in the mod save folder.";
    FLAlertLayer::create("Bismuth GLES test", summary, "OK")->show();
}

class $modify(BismuthProbeMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init())
            return false;

        /*
            Showing the popup right here doesn't work: MenuLayer::init runs while the game is
            still switching scenes, so the popup would be added to a scene that is about to go
            away. Wait a few seconds and show it from the main thread instead.
        */
        static bool scheduled = false;
        if (!scheduled) {
            scheduled = true;
            std::thread([] {
                std::this_thread::sleep_for(std::chrono::seconds(4));
                Loader::get()->queueInMainThread([] {
                    runProbe();
                });
            }).detach();
        }

        return true;
    }
};
