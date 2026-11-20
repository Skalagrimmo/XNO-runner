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
- Restart with the original seed.
- SDL application using the pinned Permission Denied Native OpenGL renderer and its existing Android touch HUD.
- Fixed 60 Hz simulation loop, separate from rendering.
- Desktop keyboard controls and Android virtual-stick/sprint controls.

## Deliberate limits

The existing corridor is reused as a continuous visual environment. This first pass does not yet draw obstacle meshes or a dedicated score/hazard HUD. Desktop window title reports run metrics and the next blocked lane; Android log output is diagnostic only. Therefore this is an engineering prototype, not yet a fair player-facing release candidate.

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

The smoke test covers deterministic obstacle generation, inert ready state, fixed-step progression, safe obstacle pass, collision, and restart. It is a native simulation test and does not substitute for a device playtest.
