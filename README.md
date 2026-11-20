# XNO Runner

**Working title.** An Android-first vector-corridor runner based on the native rendering and Android foundation of [Permission Denied — Native Engine](https://github.com/Skalagrimmo/Permission-Denied-Native), with a future optional Nano (XNO) skill-reward layer.

**Status:** pre-alpha / repository bootstrap. This repository starts with project documentation and a pinned upstream engine reference. It does not yet contain a playable runner, a release APK, a backend, a wallet, or live XNO payouts.

## Why this is separate

The existing Permission Denied corridor is the starting point, not a reason to turn that game into a runner. XNO Runner owns its runner mechanics, obstacle model, scoring, progression, presentation, and future reward integration. The original project remains unchanged.

## Technical starting point

The upstream reference is pinned at commit c536555d78f6ed8753b9dc707f7c7f18f2058d26 of Permission-Denied-Native. It includes:

- Native C++20 simulation and renderer code.
- SDL3 platform lifecycle and CMake build.
- OpenGL on desktop and OpenGL ES on Android.
- A perspective wireframe corridor, vector HUD accents, and world-space 5×7 ASCII signage.
- An Android arm64-v8a target with a thin SDL Activity shell.

The renderer-validation corridor is not yet an actual generated game world and has no runner obstacle/collision system. This repository must turn it into one deliberately and incrementally.

## Project documents

- Project brief: product goal, gameplay assumptions, and initial scope.
- Architecture: boundaries between simulation, renderer, platform, and reward service.
- Technical baseline: verified facts about the upstream engine.
- Roadmap: milestone gates from prototype to optional reward integration.
- XNO rewards: future reward-service contract and security boundaries.

## First principle

**The game must remain fully playable without a wallet or an internet connection.** Nano rewards are optional, separately funded, verified outside the client, and never processed inside the render or simulation frame loop.
