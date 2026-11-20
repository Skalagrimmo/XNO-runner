# M1 — Runner Prototype

**Branch:** `m1-runner-prototype`  
**Status:** first native simulation + corridor integration pass  
**Date:** 2026-11-20

## Implemented

- Independent `xno::runner::RunnerGame` simulation.
- Explicit ready / running / crashed states.
- Deterministic SplitMix64 obstacle generation with a per-run seed.
- Three lanes with smooth transitions and bounds.
- Automatic forward movement and a gradual speed ramp.
- Hold-to-sprint multiplier.
- Distance-derived score and obstacle-clear bonus.
- Collision detection against a blocked lane.
- Deterministic alternation of FOLD_GATE and BIT_CHASM hazard families without consuming extra obstacle RNG state.
- Restart with the original seed.
- A runner-local OpenGL vector pass for articulated FOLD_GATE arms, dark BIT_CHASM insets, falling floor plates, and warning edges.
- A BIT_DISINTEGRATION crash effect whose fragments are derived from run seed and collision obstacle ID.
- A crash presentation timer that advances while world distance, score, and simulation ticks remain frozen.
- SDL application using the pinned Permission Denied Native corridor renderer; the runner draws its own touch overlay after hazards.
- Fixed 60 Hz simulation loop, separate from rendering.
- Desktop keyboard controls and Android virtual-stick/sprint controls.

## Deliberate limits

The existing corridor remains the continuous environment. Hazard geometry is implemented in a separate runner-owned OpenGL pass using the corridor's camera projection; no source inside the pinned upstream submodule is changed. Desktop window title reports run metrics and the next hazard. A dedicated in-game score/lane HUD is still pending, and the new render pass has not yet been validated on the SM-J710F or another Android device. This remains an engineering prototype, not a release candidate.

No wallet, Nano RPC, work-generation, network permission, backend, or reward transaction code is included.

## Build targets

Desktop:

    git submodule update --init --recursive
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --config Release
    ctest --test-dir build -C Release --output-on-failure

Android (Windows):

    $env:ANDROID_SDK_ROOT = "$env:LOCALAPPDATA\Android\Sdk"
    $env:ANDROID_HOME = $env:ANDROID_SDK_ROOT
    .\upstream\permission-denied-native\android\gradlew.bat -p android assembleDebug

## Controls

- SPACE: start/restart.
- A / LEFT: move one lane left.
- D / RIGHT: move one lane right.
- SHIFT: sprint.
- Android: lower-left stick left/right changes lanes; lower-right ring sprints; tap starts/restarts.

## Test coverage

The smoke test covers deterministic obstacle placement and hazard-family order, inert ready state, fixed-step progression, safe obstacle pass, collision, frozen world state during the crash effect, and restart. The current core workflow compiles the simulation and tests; the desktop-build workflow compiles the full renderer/application. Neither substitutes for a device playtest.
