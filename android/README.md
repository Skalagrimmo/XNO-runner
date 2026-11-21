# Android target

The Android app uses the pinned SDL3 Java Activity sources from the Permission Denied Native submodule. This repository supplies its own package identity, manifest, Gradle project, CMake orchestration, and native runner entry point.

## Build on Windows

Requirements are inherited from the verified upstream toolchain:

- JDK 21
- Android SDK Platform 36
- NDK 28.2.13676358
- CMake 3.31.5
- Gradle wrapper 9.3.1 / Android Gradle Plugin 9.1.1
- ABI: arm64-v8a

From the repository root, initialize the submodule once:

    git submodule update --init --recursive

Set the SDK path in PowerShell:

    $env:ANDROID_SDK_ROOT = "$env:LOCALAPPDATA\Android\Sdk"
    $env:ANDROID_HOME = $env:ANDROID_SDK_ROOT

Use the wrapper shipped in the pinned upstream reference, but point Gradle at this repository's Android project:

    .\upstream\permission-denied-native\android\gradlew.bat -p android assembleDebug

If SDL3 source is already checked out locally, avoid another FetchContent clone:

    .\upstream\permission-denied-native\android\gradlew.bat -p android assembleDebug -PSDL3_SOURCE_DIR="C:/path/to/SDL"

APK output:

    android/app/build/outputs/apk/debug/app-debug.apk

Debug application ID: com.skalagrimmo.xnorunner.dev

## Input

- Desktop: SPACE starts/restarts; A/LEFT and D/RIGHT change lanes; E performs the contextual action; ESC exits.
- Android: drag the lower-left virtual stick left/right to change lanes; tap the lower-right action ring; tap to start/restart.

BIT_CHASM is a full-width traversal hazard. The first action press anchors a temporary plate in the player's current lane and blinks onto it. A second press blinks to solid ground beyond the gap. The plate collapses after a short fixed-time window; missing the second press causes a fall. FOLD_GATE remains a lane-avoidance hazard. Both hazard families are rendered in the runner-owned vector pass.

The runner-owned 5×7 vector HUD is rendered on desktop and Android without platform fonts or image assets. It shows distance, score, lane, the next hazard and its distance, plus ANCHOR/BLINK prompts and the live plate countdown.

On application exit, the app writes a deterministic `last-run.xnr` trace to the SDL preference directory. It contains the starting seed, config, fixed-step input events, and one-second state-hash checkpoints. The debug package can expose this file through `run-as`; see the root README for the ADB extraction command.
