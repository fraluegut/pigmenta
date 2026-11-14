cmake_minimum_required(VERSION 3.22.1)
project(pigmenta_core VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(pigmenta_core STATIC
    src/color_spaces.cpp
    src/decorrelation.cpp
    src/filters.cpp
)

target_include_directories(pigmenta_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(pigmenta_core PRIVATE
    -Wall -Wextra -Wno-unused-parameter
    -ffast-math
    -O2
)

set_target_properties(pigmenta_core PROPERTIES POSITION_INDEPENDENT_CODE ON)

# ── Unit tests ────────────────────────────────────────────────────────────────
# Off by default so nothing outside this repo's own top-level build (the
# Android NDK build, a standalone cli/ build, etc.) ever pulls in Catch2.
option(PIGMENTA_BUILD_TESTS "Build pigmenta_core unit tests (Catch2)" OFF)
option(PIGMENTA_COVERAGE "Instrument pigmenta_core + its tests for gcov coverage" OFF)

if (PIGMENTA_COVERAGE)
    target_compile_options(pigmenta_core PRIVATE --coverage -O0 -g)
    target_link_options(pigmenta_core PRIVATE --coverage)
endif()

if (PIGMENTA_BUILD_TESTS)
    include(FetchContent)
    FetchContent_Declare(
        catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.6.0
    )
    FetchContent_MakeAvailable(catch2)

    add_executable(pigmenta_core_tests
        src/color_spaces_test.cpp
        src/decorrelation_test.cpp
        src/filters_test.cpp
        include/pigmenta/matrix3x3_test.cpp
    )
    target_link_libraries(pigmenta_core_tests PRIVATE pigmenta_core Catch2::Catch2WithMain)
    target_include_directories(pigmenta_core_tests PRIVATE src)

    if (PIGMENTA_COVERAGE)
        target_compile_options(pigmenta_core_tests PRIVATE --coverage -O0 -g)
        target_link_options(pigmenta_core_tests PRIVATE --coverage)
    endif()

    include(CTest)
    include(${catch2_SOURCE_DIR}/extras/Catch.cmake)
    catch_discover_tests(pigmenta_core_tests)
endif()
