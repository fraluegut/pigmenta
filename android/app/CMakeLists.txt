cmake_minimum_required(VERSION 3.22.1)
project("pigmenta_engine" VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../core pigmenta_core)

add_library(
    pigmenta-engine
    SHARED
    ${CMAKE_CURRENT_SOURCE_DIR}/../../bindings/jni/pigmenta_jni.cpp
)

find_library(log-lib log)
find_library(jnigraphics-lib jnigraphics)

target_link_libraries(
    pigmenta-engine
    pigmenta_core
    ${log-lib}
    ${jnigraphics-lib}
)

target_compile_options(pigmenta-engine PRIVATE
    -Wall -Wextra -Wno-unused-parameter
    -O2
)
