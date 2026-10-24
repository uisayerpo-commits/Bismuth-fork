cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if ("${CMAKE_SYSTEM_NAME}" STREQUAL "iOS" OR IOS)
    set(CMAKE_OSX_ARCHITECTURES "arm64")
else()
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
endif()
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

project(Bismuth VERSION 1.0.0)

# I want to use `geode build` with clang but this is the only way to do it :(
message(STATUS "Found Geode: $ENV{CMAKE_C_COMPILER}")
if (DEFINED ENV{CMAKE_C_COMPILER})
    set(CMAKE_C_COMPILER $ENV{CMAKE_C_COMPILER})
endif()
if (DEFINED ENV{CMAKE_CXX_COMPILER})
    set(CMAKE_CXX_COMPILER $ENV{CMAKE_CXX_COMPILER})
endif()

# The renderer was written for desktop OpenGL (Windows). This fork builds for Android only,
# the OpenGL ES port is described in ANDROID_PORT.md. The Windows branch is kept as it was.
if (ANDROID OR "${CMAKE_SYSTEM_NAME}" STREQUAL "Android")
    set(BISMUTH_ANDROID ON)
    file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)
    # unused:       dead code
    # android/probe, android/smoketest: the GPU probe and smoke test, they were run already
    # profiler:     hooks a lot of engine functions, only useful on desktop
    # HotKey, main: keyboard hotkeys and an empty hook, no keyboard on a phone
    list(FILTER SOURCES EXCLUDE REGEX "src/unused/|src/android/probe\\.cpp$|src/android/smoketest\\.cpp$|src/profiler\\.cpp$|src/HotKey\\.cpp$|src/main\\.cpp$")
else()
    set(BISMUTH_ANDROID OFF)
    # Add all source files inside src (recursively), except the Android ones
    file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)
    list(FILTER SOURCES EXCLUDE REGEX "src/android/")
endif()

# Set up the mod binary
add_library(${PROJECT_NAME} SHARED ${SOURCES})

# Not sure about this...............
set(CMAKE_BUILD_TYPE RelWithDebInfo)
# set(CMAKE_CXX_FLAGS_RELEASE "-O3")

# Libraries
set(ENABLE_OPT 0)
set(ENABLE_GLSLANG_BINARIES OFF)

include(FetchContent)
FetchContent_Declare(
    glm
    GIT_REPOSITORY  https://github.com/g-truc/glm.git
    GIT_TAG         0af55ccecd98d4e5a8d1fad7de25ba429d60e863
)

if (BISMUTH_ANDROID)
    # glslang is not needed, the shaders are expanded by hand (see Shader.cpp)
    FetchContent_MakeAvailable(glm)
    target_link_libraries(${PROJECT_NAME} glm::glm)
    target_include_directories(${PROJECT_NAME} PRIVATE src)

    # While porting: report the errors of every file in a single build (see tools/keepgoing.sh).
    # Switch this off once the whole mod compiles.
    option(BISMUTH_KEEP_GOING "Keep compiling after an error" ON)
    if (BISMUTH_KEEP_GOING)
        set_target_properties(${PROJECT_NAME} PROPERTIES
            RULE_LAUNCH_COMPILE "sh ${CMAKE_CURRENT_SOURCE_DIR}/tools/keepgoing.sh"
        )
    endif()
else()
    FetchContent_Declare(glslang GIT_REPOSITORY https://github.com/KhronosGroup/glslang.git GIT_TAG vulkan-sdk-1.4.335)
    FetchContent_MakeAvailable(glm glslang)

    target_link_libraries(${PROJECT_NAME} glm::glm glslang::glslang)
    target_include_directories(${PROJECT_NAME} PRIVATE src)
endif()

# Other Geode stuff

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable to point to Geode")
else()
    message(STATUS "Found Geode: $ENV{GEODE_SDK}")
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

# Set up dependencies, resources, and link Geode.
setup_geode_mod(${PROJECT_NAME})
