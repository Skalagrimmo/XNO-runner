import org.gradle.api.GradleException

plugins {
    id("com.android.application")
}

val sdkHome = providers.gradleProperty("SDL_ANDROID_HOME")
    .orElse(providers.environmentVariable("ANDROID_SDK_ROOT"))
    .orElse(providers.environmentVariable("ANDROID_HOME"))
    .getOrElse("")
val sdl3SourceDir = providers.gradleProperty("SDL3_SOURCE_DIR").orNull

if (sdkHome.isBlank()) {
    throw GradleException("Set SDL_ANDROID_HOME or ANDROID_SDK_ROOT to the Android SDK directory.")
}

android {
    namespace = "com.skalagrimmo.xnorunner"
    compileSdk = 36
    ndkVersion = "28.2.13676358"

    defaultConfig {
        applicationId = "com.skalagrimmo.xnorunner"
        minSdk = 24
        targetSdk = 35
        versionCode = 1
        versionName = "0.1.0-runner"

        ndk {
            abiFilters += listOf("arm64-v8a")
        }

        externalNativeBuild {
            cmake {
                arguments += buildList {
                    add("-DANDROID_PLATFORM=android-24")
                    add("-DANDROID_STL=c++_shared")
                    if (!sdl3SourceDir.isNullOrBlank()) {
                        add("-DFETCHCONTENT_SOURCE_DIR_SDL3=$sdl3SourceDir")
                    }
                }
                cppFlags += listOf("-std=c++20")
            }
        }
    }

    externalNativeBuild {
        cmake {
            // This repository's root CMakeLists orchestrates the pinned engine submodule.
            path = file("../../CMakeLists.txt")
            version = "3.31.5"
        }
    }

    sourceSets["main"].java.srcDir(
        file("../../upstream/permission-denied-native/android/app/src/main/java")
    )

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }

    buildTypes {
        debug {
            applicationIdSuffix = ".dev"
        }
        release {
            isMinifyEnabled = false
        }
    }

    lint {
        abortOnError = false
    }
}
