# XNO Runner

**Working title.** An Android-first vector-corridor runner based on the native rendering and Android foundation of [Permission Denied — Native Engine](https://github.com/Skalagrimmo/Permission-Denied-Native), with a future optional Nano (XNO) skill-reward layer.

**Status:** M1 engineering prototype on branch `m1-runner-prototype`. The repository now contains an independent deterministic runner simulation, an SDL application entry point, desktop/Android build orchestration, and native smoke tests. It is not yet a player-ready release: vector obstacle meshes and an on-screen status panel remain open.

## Why this is separate

The existing Permission Denied corridor is the starting point, not a reason to turn that game into a runner. XNO Runner owns its runner mechanics, obstacle model, scoring, progression, presentation, and future reward integration. The original project remains unchanged.

## Technical starting point

The upstream reference is pinned at commit `c536555d78f6ed8753b9dc707f7c7f18f2058d26` of Permission-Denied-Native. It includes:

- Native C++20 simulation and renderer code.
- SDL3 platform lifecycle and CMake build.
- OpenGL on desktop and OpenGL ES on Android.
- A perspective wireframe corridor, vector HUD accents, and world-space 5×7 ASCII signage.
- An Android arm64-v8a target with a thin SDL Activity shell.

The M1 branch adds its own `xno::runner::RunnerGame` simulation: deterministic obstacles, three lanes, smooth lane transitions, forward movement, sprint, distance/score, collision, and restart. A dedicated SDL application maps keyboard and Android touch input to the runner while reusing the pinned OpenGL corridor renderer.

## Current controls

- Desktop: SPACE starts/restarts; A/LEFT and D/RIGHT change lanes; SHIFT sprints; ESC exits.
- Android: tap starts/restarts; drag the lower-left virtual stick left/right to change lanes; hold the lower-right ring to sprint.

## Build and test

Initialize the upstream engine:

    git submodule update --init --recursive

Desktop:

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --config Release
    ctest --test-dir build -C Release --output-on-failure

Android build notes are in [android/README.md](android/README.md).

## Current limitations

The runner currently reuses the corridor as its continuous visual environment. Obstacle collision is implemented and tested in the simulation, but dedicated 3D obstacle meshes and a visible score/next-hazard HUD are not finished. Desktop diagnostics appear in the window title; Android logging is diagnostic, not a player-facing UI.

No wallet, Nano RPC, proof-of-work, backend, internet permission, or payout code is present. The game is designed to remain offline and playable without XNO integration.

## Project documents

- [Project brief](docs/PROJECT_BRIEF.md)
- [Architecture](docs/ARCHITECTURE.md)
- [Verified upstream baseline](docs/TECHNICAL_BASELINE.md)
- [M1 prototype scope](docs/M1_RUNNER_PROTOTYPE.md)
- [Roadmap](docs/ROADMAP.md)
- [Future XNO rewards design](docs/XNO_REWARDS.md)
