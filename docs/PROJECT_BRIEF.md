# Project Brief — XNO Runner

**Document status:** initial proposal, 2026-11-20  
**Project stage:** repository/bootstrap only  
**Working title:** XNO Runner

## Product idea

Create an original Android-first runner using the vector corridor already prototyped in Permission Denied Native. The corridor is the visual and technical foundation: evolve it into a traversable, continuous game space with readable hazards, route segments, and responsive movement.

The first playable version must be self-contained and offline-capable. Nano (XNO) rewards are a later, optional layer for verified skill-based runs.

## Goals

1. Reuse the native C++20, SDL3, OpenGL/OpenGL ES, and Android corridor technology rather than restarting in another engine.
2. Keep the runner in its own repository and APK identity.
3. Turn the renderer-validation corridor into an actual game world with forward progression, obstacles, collision, and repeatable challenge generation.
4. Keep simulation deterministic where practical so runs can be reproduced and checked.
5. Remain responsive on modest Android hardware. The existing Samsung SM-J710F test is a baseline to preserve, not a performance guarantee for the completed game.
6. Design a future XNO reward boundary without coupling gameplay to Nano RPC, wallet operations, or network availability.

## Non-goals

- No changes to Permission Denied Native's gameplay or release branch.
- No changes to StackCatch.
- No copying of Nano SkyRun source code, artwork, branding, or proprietary assets.
- No blockchain transactions in the Android client.
- No claim that gameplay mines XNO. Rewards, if introduced, come from a transparently funded pool.
- No promise of guaranteed rewards or a fixed earning rate.

## Initial gameplay direction

The initial candidate is a first-person or near-first-person run through a long vector corridor matching the existing camera and renderer. The player starts a seeded run, moves through procedural or prebuilt corridor segments, reads and avoids hazards, earns distance/score, and receives a local result.

These choices remain open until a playable test is compared on target hardware: fixed lanes versus free lateral movement; jump/slide versus dodge-only; collectible types; camera sway; run duration; scoring multipliers; and endless versus stage-based structure.

## Product requirements

- Android is the primary target; desktop is a development and regression target.
- Runs work without network access.
- Gameplay simulation is independent from rendering and Android lifecycle code.
- A seed and ruleset version reproduce the same challenge sequence.
- Collision and scoring are unit-testable without an OpenGL context.
- Pause/resume and interrupted-run handling cannot silently create a valid reward result.
- Visual readability on small screens is more important than decorative vector density.
- The game remains playable if XNO services are unavailable or disabled.

## First playable success criteria

- A complete start, control, fail, result, and restart loop exists.
- At least one obstacle family has deterministic placement and collision tests.
- Distance and score derive from simulation state, not rendered frame count.
- The same seed and ruleset produce the same obstacle sequence.
- A debug Android APK runs on the existing arm64 test device without modifying the upstream project.
