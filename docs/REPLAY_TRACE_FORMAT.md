# XNO Runner Replay Trace Format

**File extension:** \`.xnr\`  
**Format version:** 2  
**Simulation ruleset:** 2  
**Purpose:** deterministic simulation reproduction and regression diagnosis

## What a replay contains

An \`.xnr\` file is a simulation trace, not a video recording. It contains:

- The original run seed.
- The fixed simulation step (\`1/60\` second in the current application).
- The complete \`RunnerConfig\`.
- Sparse input events indexed by a monotonic frame counter.
- State-hash checkpoints, normally once per 60 fixed steps.

A frame with no recorded input event is replayed with an empty \`RunnerInput\`. This keeps files compact while preserving the exact order of player inputs. The global frame counter continues across crash presentation and restart, even when the game's internal simulation tick counter resets.

## Text layout

The format is plain UTF-8 text, one record per line:

    XNO_RUNNER_REPLAY 2
    ruleset 2
    seed 636...
    fixed_delta 0.0166666675
    config <14 float values>
    frames <total frame count>
    events <event count>
    event <zero-based frame> <lane_delta> <start> <restart> <action>
    checkpoints <checkpoint count>
    checkpoint <completed frame count> <64-bit state hash>
    END

The 14 configuration values (including the 12-second speed-ramp delay) follow the \`RunnerConfig\` member order in \`runner_game.hpp\`. Boolean event fields are \`0\` or \`1\`. Event frame indexes are zero-based; a checkpoint at frame count 60 describes the state after the first 60 calls to \`RunnerGame::step\`.

Ruleset 2 uses a 34 m opening runway, randomized 28–40 m obstacle spacing, a 12-second base-speed plateau followed by a 0.065 m/s-per-second speed ramp, and a seeded 28% BIT_CHASM chance. At least two FOLD_GATE obstacles separate consecutive chasms.

The writer uses enough decimal precision to round-trip each 32-bit float. The reader rejects unknown format/ruleset versions, malformed fields, out-of-range events, and unordered checkpoints.

## Determinism checks

\`RunnerGame::deterministic_hash()\` hashes explicit fields rather than raw struct memory. It includes:

- All simulation configuration values.
- Run seed and random-generator state.
- Next obstacle ID, spawn distance, and the BIT_CHASM spacing cooldown.
- Every player/run state field, including the BIT_CHASM plate timer.
- The current obstacle list and each obstacle's resolution state.

At each checkpoint the verifier constructs a fresh \`RunnerGame\`, applies the recorded inputs at the recorded fixed-step frames, and compares the resulting 64-bit hash. On the first mismatch it reports the exact completed frame count plus expected and actual hashes.

This checks simulation state and future obstacle-generation state. It does not hash rendered pixels, GPU state, touch-device coordinates, or audio output.

## Capture and verification

The SDL app writes \`last-run.xnr\` to its app-specific preference directory when it exits. The Android debug build keeps that file in app-private storage; the root README describes extraction with \`adb\` and \`run-as\`.

On desktop, verify an exported trace with:

    xno_runner_replay_verify path/to/last-run.xnr

The verifier exits with code \`0\` when all checkpoints match, \`2\` on the first deterministic mismatch, and a non-zero input/read error code for malformed or unavailable files.

## Versioning policy

Bump \`format_version\` when the \`.xnr\` serialization layout changes. Bump \`ruleset_version\` whenever a simulation change alters how an existing input stream is interpreted or changes deterministic state evolution. A replay with an unsupported version is rejected rather than silently approximated.
