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

- Desktop: SPACE starts/restarts; A/LEFT and D/RIGHT change lanes; SHIFT sprints; ESC exits.
- Android: drag the lower-left virtual stick left/right to change lanes; hold the lower-right ring to sprint; tap to start/restart.

The first iteration reuses the existing vector corridor and touch HUD. Hazard simulation is present in the native runner core, but dedicated 3D hazard meshes and an on-screen score/status panel are still pending.
