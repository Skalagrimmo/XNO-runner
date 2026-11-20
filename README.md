# XNO Runner

**Working title.** An Android-first vector-corridor runner based on the native rendering and Android foundation of [Permission Denied — Native Engine](https://github.com/Skalagrimmo/Permission-Denied-Native), with a future optional Nano (XNO) skill-reward layer.

**Status:** M1 engineering prototype on branch `m1-runner-prototype`. The repository contains an independent deterministic runner simulation, a first-person SDL corridor application, Android/desktop build orchestration, two rendered hazard families, and native smoke tests. The next validation step is direct device playtesting and control tuning.

## Why this is separate

The existing Permission Denied corridor is the starting point, not a reason to turn that game into a runner. XNO Runner owns its runner mechanics, obstacle model, scoring, progression, presentation, and future reward integration. The original project remains unchanged.

## Technical starting point

The upstream reference is pinned at commit `c536555d78f6ed8753b9dc707f7c7f18f2058d26` of Permission-Denied-Native. It includes:

- Native C++20 simulation and renderer code.
- SDL3 platform lifecycle and CMake build.
- OpenGL on desktop and OpenGL ES on Android.
- A perspective wireframe corridor, vector HUD accents, and world-space 5×7 ASCII signage.
- An Android arm64-v8a target with a thin SDL Activity shell.

The M1 branch adds its own `xno::runner::RunnerGame` simulation: deterministic obstacles, three lanes, smooth lane transitions, forward movement, distance/score, collision, restart, and a contextual two-stage BIT_CHASM traversal. A dedicated SDL application maps keyboard and Android touch input to the runner while reusing the pinned OpenGL corridor renderer.

## Current controls

- Desktop: SPACE starts/restarts; A/LEFT and D/RIGHT change lanes; E performs the contextual action; ESC exits.
- Android: tap starts/restarts; drag the lower-left virtual stick left/right to change lanes; tap the lower-right action ring. Near a BIT_CHASM, tap once to anchor/blink onto a plate and tap again before it collapses to blink to solid ground.

## Build and test

Initialize the upstream engine:

    git submodule update --init --recursive

Desktop:

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --config Release
    ctest --test-dir build -C Release --output-on-failure

Android build notes are in [android/README.md](android/README.md).

## Current limitations

The runner currently reuses the corridor as its continuous visual environment and adds runner-owned vector geometry for FOLD_GATE and BIT_CHASM, plus the BIT_DISINTEGRATION failure effect. A runner-owned 5×7 vector HUD displays distance, score, lane, next hazard, and ANCHOR/BLINK prompts. Direct device performance tuning remains open. Desktop diagnostics also appear in the window title; Android uses a compact vector touch overlay.

No wallet, Nano RPC, proof-of-work, backend, internet permission, or payout code is present. The game is designed to remain offline and playable without XNO integration.

## Project documents

- [Project brief](docs/PROJECT_BRIEF.md)
- [Architecture](docs/ARCHITECTURE.md)
- [Verified upstream baseline](docs/TECHNICAL_BASELINE.md)
- [M1 prototype scope](docs/M1_RUNNER_PROTOTYPE.md)
- [Vector hazard art direction](docs/VECTOR_HAZARD_ART_DIRECTION.md)
- [Roadmap](docs/ROADMAP.md)
- [Future XNO rewards design](docs/XNO_REWARDS.md)
