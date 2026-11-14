cmake_minimum_required(VERSION 3.22.1)
project(pigmenta_desktop VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 COMPONENTS Widgets REQUIRED)

if (NOT TARGET pigmenta_core)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../core pigmenta_core)
endif()

add_executable(pigmenta-desktop
    src/main.cpp
    src/MainWindow.cpp
    src/FilterPanel.cpp
    src/DropArea.cpp
    src/CompareView.cpp
    src/Engine.cpp
    src/WelcomeDialog.cpp
    src/DocsDialog.cpp
    src/TitleBar.cpp
    src/FrameContainer.cpp
    src/Translator.cpp
    resources/resources.qrc
)

target_link_libraries(pigmenta-desktop PRIVATE pigmenta_core Qt6::Widgets)

target_compile_definitions(pigmenta-desktop PRIVATE
    PIGMENTA_VERSION="${CMAKE_PROJECT_VERSION}"
)

target_compile_options(pigmenta-desktop PRIVATE -Wall -Wextra -Wno-unused-parameter)

if (PIGMENTA_COVERAGE)
    target_link_options(pigmenta-desktop PRIVATE --coverage)
endif()

# ── Unit tests ────────────────────────────────────────────────────────────
# Only the testable logic (Translator, Engine) — not the widgets, which are
# integration-tested by hand (see docs/TESTING.md).
if (PIGMENTA_BUILD_TESTS AND TARGET Catch2::Catch2WithMain)
    add_executable(pigmenta_desktop_tests
        src/Translator_test.cpp
        src/Translator.cpp
        src/Engine_test.cpp
        src/Engine.cpp
    )
    target_link_libraries(pigmenta_desktop_tests PRIVATE pigmenta_core Qt6::Widgets Catch2::Catch2WithMain)
    target_include_directories(pigmenta_desktop_tests PRIVATE src)
    if (PIGMENTA_COVERAGE)
        target_compile_options(pigmenta_desktop_tests PRIVATE --coverage -O0 -g)
        target_link_options(pigmenta_desktop_tests PRIVATE --coverage)
    endif()
    catch_discover_tests(pigmenta_desktop_tests)
endif()
