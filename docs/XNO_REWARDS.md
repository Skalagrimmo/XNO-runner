# Future Nano (XNO) Reward Integration

**Status:** architecture notes only. No wallet, RPC client, server, payout code, or reward pool exists in this repository yet.

## 1. Principle

XNO is a reward rail, not the game's simulation engine and not a mining mechanism. Players must be able to play and retain a local result without signing in, having a wallet, or being online.

## 2. Proposed flow

1. The game completes a run and creates a record containing a unique run ID, seed, ruleset version, final state, and evidence required by the chosen validation scheme.
2. The client optionally submits the record to a backend.
3. The backend verifies eligibility, deterministic challenge data, plausibility, and replay/duplicate protections.
4. Eligible results may credit an internal ledger according to a published policy and the available reward-pool balance.
5. Payouts are batched or scheduled by a dedicated service.
6. The service tracks Nano block confirmation and marks a payout complete only after the required confirmation state.

A submitted score is not proof by itself. Anti-cheat and validation must be designed before rewards are enabled.

## 3. Key custody and client boundary

- The APK must never contain a treasury seed, private key, or signing secret.
- The client should need only a public Nano receive address if user-directed payouts are implemented.
- Signing and block publication belong in a controlled server-side service or a deliberately designed external-wallet flow.
- Never expose node wallet RPC credentials to the Android client.
- Use limited hot-wallet exposure, monitoring, payout caps, and an emergency stop if a hosted treasury is used.

## 4. Reliability and accounting

- Give every run and payout a stable unique identifier.
- Enforce idempotency: retrying a submission must not create multiple credits; retrying a payout must not create duplicate transfers.
- Keep the internal ledger separate from Nano transaction status.
- Record pending, submitted, confirmed, failed, and reconciled states explicitly.
- Do not claim a payout is complete until confirmation is verified.
- Explain reward-pool funding, eligibility, minimum payout, service costs, and suspension conditions to players.

## 5. Network and game isolation

- Never perform RPC, proof-of-work generation, wallet operations, or payout polling inside the frame loop.
- Network failure must not interrupt an active run.
- Use a test environment and test funds before any mainnet operation.
- Live rewards are optional and depend on a funded pool; they are not guaranteed income.

## 6. Open decisions

- Custodial internal balance versus direct user-address payout.
- Minimum payout threshold and payout cadence.
- Test network/provider and work-generation strategy.
- Exact run-validation method and acceptable false-positive/false-negative rates.
- Eligibility rules, region availability, privacy, and abuse-response policy.

These decisions are deliberately unresolved at the bootstrap stage.
