# Proposed Architecture

**Status:** M1 first pass implemented; renderer and app boundaries are in place.

## 1. Layering

    Android APK / Desktop build
              |
              v
    SDL platform shell (lifecycle, input, clock)
              |
              v
    XNO Runner application (ready / run / crash / restart)
              |
              +----------------------------+
              |                            |
              v                            v
    Runner simulation                Upstream vector renderer
    (seed, lanes,                    (corridor, vector HUD,
     obstacles, collision,            perspective projection)
     distance, score)
              |
              v
    Local run result / future replay record
              |
              v
    Optional future reward adapter
              |
              v
    Remote validation + reward ledger + Nano payout service

## 2. Ownership rules

### Runner simulation

Implemented in `runner/core/`. Owns seed, run state, player lane, forward distance, obstacle schedule, collision, sprint, and score. It has no SDL, OpenGL, Android, or Nano dependency.

### Vector renderer

The M1 application reuses the pinned Permission Denied Native OpenGL/OpenGL ES renderer and feeds it a render-only camera state. The renderer remains read-only relative to the simulation. Dedicated runner hazard meshes and status overlays are not implemented yet.

### Platform shell

Implemented in `runner/platform/`. Owns SDL window/context creation, events, Android touch input, desktop keyboard input, a fixed 60 Hz simulation accumulator, and presentation timing. Platform-specific code does not contain collision or scoring rules.

### Reward client adapter

Not implemented. When added, it will optionally submit completed run records when online. It must contain no private Nano keys or block-signing operation. Submission failure must never block local play.

### Reward backend (future)

Not implemented. It will validate run submissions, apply eligibility and anti-replay rules, maintain an auditable internal ledger, and coordinate payouts. It remains outside the game loop.

## 3. Clocks and determinism

- Simulation advances at a fixed 60 Hz; rendering may run at a different rate.
- Distance and score derive from simulation updates, never rendered frame count.
- Each run fixes a seed; SplitMix64 drives obstacle lane and spacing selection without relying on implementation-defined standard-library RNG behavior.
- A replay or evidence format must be defined before client scores can qualify for rewards.
- Pause/resume and interrupted-run policy remain open test tasks.

## 4. Performance

- The current upstream renderer rebuilds and CPU-projects its test corridor geometry each frame.
- Runner simulation uses a bounded obstacle look-ahead and removes resolved obstacles behind the player.
- Generate corridor segments incrementally and cache static geometry only after profiling on Android.
- Do not optimize based solely on desktop measurements.

## 5. Dependency boundary

Use the upstream submodule as a pinned reference. The CMake project links against the upstream core and renderer while supplying its own app entry point. The upstream repository and its main branch remain unchanged. When source extraction begins, document every imported component and its source commit.
