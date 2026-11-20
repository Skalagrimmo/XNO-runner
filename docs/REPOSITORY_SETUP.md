# Repository Setup

## Repository

- GitHub: https://github.com/Skalagrimmo/XNO-runner
- Default branch: main
- Visibility: public

## Clone

Clone with submodules enabled:

    git clone --recurse-submodules https://github.com/Skalagrimmo/XNO-runner.git

If the repository was cloned without submodules:

    git submodule update --init --recursive

The submodule points to the fixed Permission Denied Native baseline recorded in docs/TECHNICAL_BASELINE.md.

## Source provenance

Do not replace the pinned upstream reference with a floating main branch. When updating the submodule pointer, document why and run the relevant Android and desktop smoke tests.

## Initial development rule

The upstream repository is read-only for this project. Runner-specific changes belong in XNO-runner. When source extraction begins, explicitly record which files were imported and the upstream commit they came from.
