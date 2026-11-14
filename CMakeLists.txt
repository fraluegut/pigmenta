cmake_minimum_required(VERSION 3.22.1)
project(pigmenta VERSION 0.1.0 LANGUAGES CXX)

# Builds the native engine (core/) and the desktop CLI (cli/).
# The Android app is a separate Gradle project under android/ that builds
# core/ and bindings/jni/ via its own CMake invocation (see android/app/CMakeLists.txt).

add_subdirectory(core)
add_subdirectory(cli)

find_package(Qt6 COMPONENTS Widgets QUIET)
if (Qt6Widgets_FOUND)
    add_subdirectory(desktop)
else()
    message(STATUS "Qt6 Widgets not found — skipping desktop/ (install qt6-base-dev to build it)")
endif()
