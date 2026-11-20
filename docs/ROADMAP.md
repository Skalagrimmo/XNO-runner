# Roadmap

## M0 — Repository and design baseline

- [x] Create a separate project scaffold.
- [x] Pin Permission Denied Native to a fixed upstream commit.
- [x] Record verified renderer, Android, and toolchain facts.
- [x] Define game, reward, and platform boundaries.
- [x] Create the remote GitHub repository and publish the initial documentation.
- [ ] Decide whether the upstream engine is submoduled, vendored, or selectively extracted for the first playable build.

## M1 — Corridor runner prototype (no blockchain)

- [ ] Establish a reproducible native and Android build in this repository.
- [ ] Create a minimal runner state machine: menu, running, paused, failed, result.
- [ ] Add forward progression through deterministic corridor segments.
- [ ] Choose and implement the first movement model.
- [ ] Add one obstacle family and collision tests.
- [ ] Derive distance and score from simulation state.
- [ ] Record a deterministic run seed and ruleset version.

## M2 — Playable Android vertical slice

- [ ] Add mobile HUD and tune touch controls by direct playtesting.
- [ ] Verify lifecycle pause/resume and interrupted runs.
- [ ] Add restart flow and local best score.
- [ ] Profile CPU, GPU, and frame time on the SM-J710F baseline and one newer Android device.
- [ ] Add quality settings if measured bottlenecks require them.

## M3 — Content and replay validation

- [ ] Add more obstacle families and corridor segment patterns.
- [ ] Add replay/event-trace format and deterministic regression fixtures.
- [ ] Test repeated seeds, edge collisions, and long runs.
- [ ] Add score/ruleset versioning and migration policy.

## M4 — Reward-service prototype (test environment only)

- [ ] Specify the run-submission API and server-side verification rules.
- [ ] Implement a reward ledger and duplicate-submission protection.
- [ ] Add an explicit test-mode toggle and test payouts.
- [ ] Keep all signing keys outside the APK.
- [ ] Test confirmation tracking, retries, idempotency, and failed-payout recovery.

## M5 — Optional production XNO rewards

- [ ] Document reward-pool funding and payout policy.
- [ ] Complete threat review and abuse testing.
- [ ] Add conservative limits, monitoring, and an emergency payout pause.
- [ ] Only then consider mainnet payouts.

## Release gate

A playable, deterministic offline game must exist before any live reward integration is considered. Mainnet integration is not part of the initial scaffold.
