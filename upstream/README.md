# Upstream engine reference

The permission-denied-native directory is a Git submodule pinned to the baseline commit documented in docs/TECHNICAL_BASELINE.md.

Initialize it after cloning:

    git submodule update --init --recursive

Treat this upstream as a read-only reference while establishing the runner architecture. Do not add runner mechanics to the upstream repository.
