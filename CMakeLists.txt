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

# The renderer is written for desktop OpenGL (Windows). The Android build is being
# ported to OpenGL ES in phases. Until then it only builds src/android (see ANDROID_PORT.md).
if (ANDROID OR "${CMAKE_SYSTEM_NAME}" STREQUAL "Android")
    set(BISMUTH_ANDROID ON)
    file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/android/*.cpp)
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

if (BISMUTH_ANDROID)
    # Only the OpenGL ES probe is built for now. It needs neither glm nor glslang,
    # and it finds the GL functions at runtime so there is nothing to link.
else()
    include(FetchContent)
    FetchContent_Declare(
        glm
        GIT_REPOSITORY  https://github.com/g-truc/glm.git
        GIT_TAG         0af55ccecd98d4e5a8d1fad7de25ba429d60e863
    )
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
