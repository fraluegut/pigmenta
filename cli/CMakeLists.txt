cmake_minimum_required(VERSION 3.22.1)
project(pigmenta_cli VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (NOT TARGET pigmenta_core)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../core pigmenta_core)
endif()

add_executable(pigmenta-cli src/main.cpp src/args.cpp)

target_include_directories(pigmenta-cli PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/stb
)

target_link_libraries(pigmenta-cli PRIVATE pigmenta_core)

target_compile_options(pigmenta-cli PRIVATE -Wall -Wextra -Wno-unused-parameter -O2)

if (PIGMENTA_COVERAGE)
    target_link_options(pigmenta-cli PRIVATE --coverage)
endif()

# ── Unit tests (shares the PIGMENTA_BUILD_TESTS option + Catch2 fetched by
#    core/CMakeLists.txt; both must be configured together from the repo
#    root for this to be available). ──────────────────────────────────────
if (PIGMENTA_BUILD_TESTS AND TARGET Catch2::Catch2WithMain)
    add_executable(pigmenta_cli_tests src/args_test.cpp src/args.cpp)
    target_link_libraries(pigmenta_cli_tests PRIVATE pigmenta_core Catch2::Catch2WithMain)
    target_include_directories(pigmenta_cli_tests PRIVATE src)
    if (PIGMENTA_COVERAGE)
        target_compile_options(pigmenta_cli_tests PRIVATE --coverage -O0 -g)
        target_link_options(pigmenta_cli_tests PRIVATE --coverage)
    endif()
    catch_discover_tests(pigmenta_cli_tests)
endif()
