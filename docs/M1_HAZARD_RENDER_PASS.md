# M1 — Vector Hazard Render Pass

**Branch:** \`m1-runner-prototype\`  
**Status:** implementation pass; device validation pending  
**Date:** 2026-11-20

## Renderer boundary

The pinned \`upstream/permission-denied-native\` submodule remains unchanged. Its OpenGL renderer continues to own the corridor background and the original vector scene. XNO Runner adds \`runner/renderer/runner_hazard_renderer.cpp\`, which creates a separate OpenGL ES 2.0 / desktop OpenGL 2.1 line-and-fill pass using SDL's GL procedure lookup.

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

### BIT_DISINTEGRATION

- On a confirmed simulation collision, a short wireframe avatar flash precedes a burst of square vector fragments.
- Fragment direction and spread are derived from the run seed, collision obstacle ID, and fragment index.
- The effect uses a presentation-only timer. It does not alter distance, score, collision, or simulation tick count.

## Determinism and warning window

The two hazard families alternate by obstacle ID, not by an additional RNG call. Existing SplitMix64 lane and spacing generation therefore retains its random stream.

The visual wake begins about 14 world units before the obstacle plane and the mechanism finishes deploying around 4 units before it. The collision plane and lane rules are still determined exclusively by \`RunnerGame\`.

## Verification state

- Core smoke tests now check hazard-family ordering and that the crash effect timer advances while world simulation remains frozen.
- A Linux desktop CMake build is added to CI to compile the full SDL/OpenGL application and the new renderer.
- Android build, actual device visibility/readability, touch-overlay occlusion, and frame-time profiling remain pending.
- The M1 pass still has no wallet, Nano RPC, work-generation, payout, or online dependency.
