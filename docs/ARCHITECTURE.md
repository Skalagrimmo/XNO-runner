# Proposed Architecture

**Status:** initial boundaries; implementation not started.

## 1. Layering

    Android APK / Desktop build
              |
              v
    Platform shell (SDL lifecycle, input, audio, clock)
              |
              v
    Runner application (menu, run, pause, result flow)
              |
              +----------------------------+
              |                            |
              v                            v
    Runner simulation                Vector renderer
    (seed, movement,                 (corridor, hazards,
     obstacles, collision,            HUD, effects)
     distance, score)
              |
              v
    Run result / replay record
              |
              v
    Optional reward client adapter
              |
              v
    Remote validation + reward ledger + Nano payout service

## 2. Ownership rules

### Runner simulation

Owns the authoritative local rules for a run: seed, ruleset version, simulation time, player state, segment progression, obstacle state, collision, distance, and score. It must not depend on OpenGL or Android classes.

### Vector renderer

Consumes read-only runner state and emits visuals. Rendering must not decide collisions, add score, advance the simulation, or call blockchain APIs. It may interpolate between simulation states for smooth presentation.

### Platform shell

Owns SDL lifecycle, surface/context creation, input acquisition, pause/resume, display-size changes, and audio focus. Platform-specific code must not contain runner scoring rules.

### Reward client adapter

Optionally submits a completed run record when connectivity is available. It contains no private Nano keys, wallet seed, or block-signing operation. Submission failure never blocks local play.

### Reward backend (future)

Verifies run submissions, applies eligibility and anti-replay rules, maintains an auditable internal ledger, and coordinates payouts. It is outside the initial documentation-only bootstrap.

## 3. Clocks and determinism

- Use a fixed-step simulation clock with bounded catch-up.
- Rendering may use a variable frame rate and interpolate visual state.
- Distance and score derive from simulation state, never the number of rendered frames.
- Pause/resume is an explicit state transition; lifecycle gaps do not become free progress.
- Each run fixes a unique run ID, seed, and ruleset version.
- Use a specified PRNG and versioned generation rules.
- A replay or evidence format must be defined before a client score can qualify for rewards.

## 4. Performance

- Generate corridor segments incrementally instead of rebuilding the entire distant world every frame.
- Cache static geometry where profiling demonstrates a gain.
- Reuse transient geometry and simulation objects.
- Keep expensive effects optional and profile low/medium settings on Android.
- Do not optimize based solely on desktop measurements.

## 5. Dependency boundary

Use the upstream submodule as a pinned reference at first. The runner must eventually build reproducibly from its own repository. Sharing or extracting engine code must be an explicit milestone, not an accidental modification of Permission Denied.
