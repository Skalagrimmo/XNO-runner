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
- A contextual action input replacing sprint.
- Distance-derived score and obstacle-clear bonus.
- FOLD_GATE lane collision and full-width BIT_CHASM collision.
- Deterministic alternation of FOLD_GATE and BIT_CHASM hazard families without consuming extra obstacle RNG state.
- BIT_CHASM two-stage ANCHOR/BLINK traversal: first press stabilizes a plate in the current lane and pauses forward movement; second press teleports to solid ground; missing the fixed-time window causes a fall.
- Restart with the original seed.
- A runner-local OpenGL vector pass for articulated FOLD_GATE arms, a full-width BIT_CHASM void, falling floor plates, an anchored temporary plate, and warning edges.
- A compact 5×7 vector HUD for distance, score, lane, next hazard, contextual ANCHOR/BLINK prompts, and plate countdown; shared between desktop and Android.
- A BIT_DISINTEGRATION crash effect whose fragments are derived from run seed and collision obstacle ID.
- A crash presentation timer that advances while world distance, score, and simulation ticks remain frozen.
- SDL application using the pinned Permission Denied Native corridor renderer; the runner draws its own touch overlay after hazards.
- Fixed 60 Hz simulation loop, separate from rendering.
- Versioned sparse-input replay format with seed/config capture, one-second deterministic state-hash checkpoints, serialization/deserialization, and headless replay verification.
- Automatic saving of the latest session trace to the SDL preference directory when the app exits.
- Desktop `xno_runner_replay_verify` utility for checking exported replay traces.
- Desktop keyboard controls and Android virtual-stick/contextual-action controls.

## Deliberate limits

The existing corridor remains the continuous environment. Hazard geometry and the contextual BIT_CHASM action are implemented in separate runner-owned simulation/rendering code using the corridor's camera projection; no source inside the pinned upstream submodule is changed. The in-game vector HUD reports distance, score, lane, and next hazard; its contextual prompt changes from ANCHOR to BLINK while the player is on the temporary plate. Desktop window title remains supplemental diagnostics. The updated HUD and traversal still require direct device playtesting on the SM-J710F or another Android device. This remains an engineering prototype, not a release candidate.

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
- E: contextual ANCHOR/BLINK action for BIT_CHASM.
- Android: lower-left stick left/right changes lanes; tap the lower-right action ring. Near BIT_CHASM, tap once to anchor/blink onto the plate and tap again before it collapses to blink to solid ground; tap elsewhere to start/restart.

## Test coverage

The smoke test covers deterministic obstacle placement and hazard-family order, inert ready state, fixed-step progression, safe obstacle pass, BIT_CHASM's successful two-press traversal and timeout fall, frozen world state during the crash effect, restart, replay file round-trip, successful playback verification, and detection of a tampered input at the first divergent checkpoint. The current core workflow compiles the simulation and tests; the desktop-build workflow compiles the full renderer/application. Neither substitutes for a device playtest.
