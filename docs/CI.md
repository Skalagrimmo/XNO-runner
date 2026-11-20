# Continuous Integration

The `runner-core.yml` workflow compiles the platform-independent runner simulation and smoke test using GCC/Clang on Linux and macOS and MSVC on Windows.

The native core check does not require the SDL3 submodule because it intentionally tests only `runner/core/`. Full desktop and Android packaging remain separate validation gates requiring the pinned upstream submodule and Android toolchain.
