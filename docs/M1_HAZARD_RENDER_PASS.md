# M1 — Vector Hazard Render Pass

**Branch:** `m1-runner-prototype`  
**Status:** implementation pass; device validation pending  
**Date:** 2026-11-20

## Renderer boundary

The pinned `upstream/permission-denied-native` submodule remains unchanged. Its OpenGL renderer continues to own the corridor background and the original vector scene. XNO Runner adds `runner/renderer/runner_hazard_renderer.cpp`, which creates a separate OpenGL ES 2.0 / desktop OpenGL 2.1 line-and-fill pass using SDL's GL procedure lookup.

The runner pass mirrors the upstream camera transform and perspective constants so its world-space hazard vertices align with the floor and corridor. It owns its shaders, VBOs, and temporary geometry buffers. Touch controls are drawn after the hazards so they remain readable when a mechanism approaches the camera.

## Implemented geometry

### FOLD_GATE

- Narrow mechanical frame with side posts and top/bottom rails.
- Two articulated arms driven by distance-to-player, with hinge diamonds and warning chevrons.
- Cyan dormant geometry transitions through amber deployment to magenta/orange danger.
- Fully deployed arms form a barrier across the assigned lane.

### BIT_CHASM

- Dark inset quad occludes the original floor grid inside the blocked lane.
- Eight individual plates shift apart and descend as the player approaches.
- Broken amber/cyan edge rails and deterministic falling vector fragments communicate depth.
- It remains a lane-blocking obstacle. No jump input is required.

### RASTER_SWEEPER

- A suspended vector frame spans the corridor, with three lane indicators on its upper rail.
- A seven-band laser curtain occupies one lane at a time and sweeps left → centre → right → centre, then repeats; seed-derived direction can reverse the sequence.
- The active lane is amber during warning and shifts toward magenta as the player approaches. A short overhead trace and floor chevrons communicate direction and current danger.
- The scan begins waking about 20 world units before the collision plane. Collision is checked at the forward-plane crossing against the active lane at simulation time; the other two lanes remain safe.

### BIT_DISINTEGRATION

- On a confirmed simulation collision, a short wireframe avatar flash precedes a burst of square vector fragments.
- Fragment direction and spread are derived from the run seed, collision obstacle ID, and fragment index.
- The effect uses a presentation-only timer. It does not alter distance, score, collision, or simulation tick count.

## Determinism and warning window

The three hazard families use the existing seeded hazard roll, without consuming additional random values: BIT_CHASM occupies 28% of eligible rolls, RASTER_SWEEPER occupies the next 24%, and remaining rolls produce FOLD_GATE. After a chasm, at least two actual FOLD_GATE obstacles must spawn before another chasm can be selected. The sweeper direction and phase offset are derived from already-generated obstacle data.

FOLD_GATE and BIT_CHASM wake about 14 world units before their obstacle plane; RASTER_SWEEPER wakes about 20 units ahead to give its moving warning time to be read. The active raster lane and its collision decision are both derived from the same fixed-step simulation clock and `RunnerGame::raster_sweeper_lane`.

## Verification state

- Core smoke tests check seeded hazard generation, the two-FOLD_GATE chasm cooldown, both sweep directions and the left-centre-right-centre phase sequence, replay round-trip, and that the crash effect timer advances while world simulation remains frozen.
- A Linux desktop CMake build is added to CI to compile the full SDL/OpenGL application and the new renderer.
- Android build, actual device visibility/readability, touch-overlay occlusion, and frame-time profiling remain pending.
- The M1 pass still has no wallet, Nano RPC, work-generation, payout, or online dependency.
