#pragma once

#include "runner/core/runner_game.hpp"

#include <cstdint>
#include <iosfwd>
#include <string>
#include <vector>

namespace xno::runner {

inline constexpr std::uint32_t kRunnerReplayFormatVersion = 2;
inline constexpr std::uint32_t kRunnerReplayRulesetVersion = 2;

struct RunnerReplayEvent {
    std::uint64_t frame_index = 0;
    RunnerInput input{};
};

struct RunnerReplayCheckpoint {
    std::uint64_t frame_count = 0;
    std::uint64_t state_hash = 0;
};

struct RunnerReplay {
    std::uint32_t format_version = kRunnerReplayFormatVersion;
    std::uint32_t ruleset_version = kRunnerReplayRulesetVersion;
    std::uint64_t seed = 1;
    float fixed_delta_seconds = 1.0F / 60.0F;
    RunnerConfig config{};
    std::uint64_t frame_count = 0;
    std::vector<RunnerReplayEvent> events;
    std::vector<RunnerReplayCheckpoint> checkpoints;
};

class RunnerReplayRecorder final {
public:
    explicit RunnerReplayRecorder(
        std::uint64_t seed,
        RunnerConfig config = {},
        float fixed_delta_seconds = 1.0F / 60.0F,
        std::uint64_t checkpoint_interval = 60);

    // Records non-empty inputs sparsely, advances the game by one fixed frame,
    // and stores a deterministic state hash at each checkpoint interval.
    void step(RunnerGame& game, const RunnerInput& input);
    void finish(const RunnerGame& game);

    [[nodiscard]] const RunnerReplay& replay() const noexcept { return replay_; }

private:
    RunnerReplay replay_{};
    std::uint64_t checkpoint_interval_ = 60;
};

struct RunnerReplayVerification {
    bool success = false;
    std::uint64_t frames_processed = 0;
    std::uint64_t mismatch_frame_count = 0;
    std::uint64_t expected_hash = 0;
    std::uint64_t actual_hash = 0;
    std::string error;
};

[[nodiscard]] bool write_runner_replay(
    std::ostream& output,
    const RunnerReplay& replay,
    std::string* error = nullptr);

[[nodiscard]] bool read_runner_replay(
    std::istream& input,
    RunnerReplay& replay,
    std::string* error = nullptr);

[[nodiscard]] RunnerReplayVerification verify_runner_replay(
    const RunnerReplay& replay);

} // namespace xno::runner
