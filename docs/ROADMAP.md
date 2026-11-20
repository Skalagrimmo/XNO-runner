# Roadmap

## M0 — Repository and design baseline

- [x] Create a separate project scaffold.
- [x] Pin Permission Denied Native to a fixed upstream commit.
- [x] Record verified renderer, Android, and toolchain facts.
- [x] Define game, reward, and platform boundaries.
- [x] Create the remote GitHub repository and publish the initial documentation.
- [x] Add independent native runner simulation and deterministic smoke tests.
- [x] Add a runner-specific SDL application and build orchestration.
- [x] Add an Android application identity and Gradle/CMake target.
- [ ] Build and run desktop target from a clean clone.
- [ ] Build, install, and launch the first XNO Runner APK on the SM-J710F.

## M1 — Corridor runner prototype (no blockchain)

- [x] Establish a reproducible native and Android build definition in this repository.
- [x] Create a minimal run state machine: ready, running, crashed, restart.
- [x] Add forward progression through the existing vector corridor.
- [x] Implement three-lane movement and smooth lane transitions.
- [x] Add deterministic obstacle placement from a fixed seed.
- [x] Add obstacle collision and run result state.
- [x] Derive distance and score from simulation state, not rendered frame count.
- [x] Define an original vector-hazard art direction: unfolding mechanisms, bit chasms, iris gates, and a digital disintegration fail effect.
- [ ] Render the FOLD_GATE obstacle as the first animated vector hazard.
- [ ] Render BIT_CHASM floor plates and falling fragments.
- [ ] Add the BIT_DISINTEGRATION crash effect.
- [ ] Render obstacle meshes and animation states in the vector corridor.
- [ ] Add an on-screen score, lane, and next-hazard indicator.
- [ ] Add replay/event-trace format and deterministic regression fixtures.
- [ ] Tune touch controls by direct playtesting.
- [ ] Verify lifecycle pause/resume and interrupted runs.
- [ ] Profile CPU, GPU, and frame time on the SM-J710F baseline and one newer Android device.
- [ ] Add quality settings if measured bottlenecks require them.

## M2 — Content and validation

- [ ] Add multiple obstacle families and corridor segment patterns.
- [ ] Add deterministic replay export and verification tests.
- [ ] Test long runs, edge collisions, and seed reproducibility.
- [ ] Add score/ruleset versioning and migration policy.

## M3 — Reward-service prototype (test environment only)

- [ ] Specify the run-submission API and server-side verification rules.
- [ ] Implement a reward ledger and duplicate-submission protection.
- [ ] Add an explicit test-mode toggle and test payouts.
- [ ] Keep all signing keys outside the APK.
- [ ] Test confirmation tracking, retries, idempotency, and failed-payout recovery.

## M4 — Optional production XNO rewards

- [ ] Document reward-pool funding and payout policy.
- [ ] Complete threat review and abuse testing.
- [ ] Add conservative limits, monitoring, and an emergency payout pause.
- [ ] Only then consider mainnet payouts.

## Release gate

A playable, deterministic offline game with visible hazards and a clear result UI must exist before any live reward integration is considered. Mainnet integration is not part of the initial prototype.
