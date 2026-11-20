# Technical Baseline — Permission Denied Native

**Reference repository:** https://github.com/Skalagrimmo/Permission-Denied-Native  
**Pinned commit:** c536555d78f6ed8753b9dc707f7c7f18f2058d26  
**Commit date:** 2026-11-18  
**Recorded upstream version:** native C++ engine 0.1.x; vector/ASCII corridor prototype.

## 1. Verified technology

- C++20 shared simulation and renderer code.
- SDL3 3.4.20 fetched by CMake FetchContent.
- CMake is the native build system.
- Desktop uses OpenGL; Android requests an OpenGL ES 2.0 context.
- Android uses a thin Java SDL Activity shell; gameplay and rendering remain native C++.
- The documented Android ABI is arm64-v8a with minSdk 24.
- Android packages libSDL3.so and libmain.so; desktop retains a static SDL3 build.

## 2. Existing vector corridor

The corridor implementation lives primarily in:

- renderer/src/vector_scene.cpp
- renderer/src/vector_scene.hpp
- renderer/src/opengl_renderer.cpp
- core/include/pd/core/game.hpp

The current test scene includes a perspective floor grid, paired guide rails, corridor walls and braces, overhead conduits, columns, portal frames, terminals, world-space 5×7 ASCII labels, and vector touch-HUD accents.

The renderer transforms world points into camera space, clips segments to near/far planes, projects them to normalized device coordinates, and draws depth-tested GL_LINES. Geometry is currently rebuilt and CPU-projected each frame.

**Important limitation:** upstream documentation describes this as renderer-validation geometry, not a generated game world. It is not connected to runner collision, obstacle generation, or a complete game loop.

## 3. Android input and device evidence

The upstream native input state has movement axes, look deltas, and a sprint flag. The documented touch layout separates the lower-left movement stick from right-side camera look and the sprint target.

The upstream Android bootstrap report records a successful install and launch on a physical Samsung SM-J710F (Android 10, Exynos 7870, Mali-T830). The vector/ASCII corridor was subsequently installed and launched on that device. These are renderer/input smoke-test results, not a measured performance guarantee for the future runner.

## 4. Reuse policy

1. Keep the upstream repository unchanged.
2. Treat the pinned submodule as a read-only reference while the runner architecture is established.
3. When runner code is ready, explicitly choose which source files become shared modules and which are adapted/copied here.
4. Record the source commit for every imported component.
5. Do not depend on hidden local build paths from the upstream developer machine.

## 5. Extraction candidates

- Core timing, input, and game-state patterns from core/.
- SDL lifecycle and native Android entry integration from platform/sdl/ and android/.
- Vector projection and GL line rendering from renderer/.
- Build conventions from the root CMakeLists.txt and Android Gradle project.

The runner owns its game rules, obstacle model, scoring, and session record. Those systems do not belong in Permission Denied.
