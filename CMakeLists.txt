cmake_minimum_required(VERSION 3.24)

project(XNORunner
    VERSION 0.1.0
    DESCRIPTION "Android-first vector corridor runner with optional Nano rewards"
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/upstream/permission-denied-native/CMakeLists.txt")
    message(FATAL_ERROR
        "Permission Denied Native submodule is missing. Run: "
        "git submodule update --init --recursive")
endif()

# Keep upstream core, renderer, SDL setup, and regression test targets available.
# Its desktop app is not needed by this repository; XNO Runner provides its own.
add_subdirectory(upstream/permission-denied-native upstream-engine-build)
if(NOT ANDROID AND TARGET pd_desktop)
    set_property(TARGET pd_desktop PROPERTY EXCLUDE_FROM_ALL TRUE)
endif()

add_library(xno_runner_core STATIC
    runner/core/runner_game.cpp
)
target_include_directories(xno_runner_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_features(xno_runner_core PUBLIC cxx_std_20)

if(ANDROID)
    if(NOT TARGET main)
        message(FATAL_ERROR "Expected the upstream SDL Android shared target 'main'.")
    endif()

    # Keep upstream SDL/NDK/GL configuration and native linkage, but replace the
    # Permission Denied entry point with the runner application.
    set_property(TARGET main PROPERTY SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/runner/platform/runner_main.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/runner/platform/sdl_runner_application.cpp"
    )
    target_include_directories(main PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
    target_link_libraries(main PRIVATE xno_runner_core)
    set_target_properties(main PROPERTIES OUTPUT_NAME "main")
    message(STATUS "XNO Runner target: Android arm64-v8a")
else()
    add_executable(xno_runner_desktop
        runner/platform/runner_main.cpp
        runner/platform/sdl_runner_application.cpp
    )
    target_include_directories(xno_runner_desktop PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
    target_link_libraries(xno_runner_desktop PRIVATE xno_runner_core pd_core pd_renderer)
    set_target_properties(xno_runner_desktop PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
    message(STATUS "XNO Runner target: desktop")
endif()

if(MSVC)
    target_compile_options(xno_runner_core PRIVATE /W4 /permissive- /Zc:__cplusplus)
    if(TARGET xno_runner_desktop)
        target_compile_options(xno_runner_desktop PRIVATE /W4 /permissive- /Zc:__cplusplus)
    endif()
else()
    target_compile_options(xno_runner_core PRIVATE -Wall -Wextra -Wpedantic)
    if(TARGET xno_runner_desktop)
        target_compile_options(xno_runner_desktop PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(xno_runner_tests runner/tests/runner_smoke.cpp)
    target_link_libraries(xno_runner_tests PRIVATE xno_runner_core)
    add_test(NAME xno_runner_core_smoke COMMAND xno_runner_tests)
endif()
