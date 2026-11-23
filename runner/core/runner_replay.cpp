#include "runner/core/runner_replay.hpp"

#include <array>
#include <cmath>
#include <iomanip>
#include <istream>
#include <limits>
#include <locale>
#include <ostream>
#include <sstream>
#include <utility>

namespace xno::runner {
namespace {

constexpr float kMaximumReplayDelta = 0.1F;

void set_error(std::string* error, const char* message) {
    if (error != nullptr) *error = message;
}

std::array<float, 14> config_values(const RunnerConfig& config) noexcept {
    return {
        config.base_speed,
        config.max_speed,
        config.speed_ramp_delay_seconds,
        config.speed_ramp_per_second,
        config.lane_change_speed,
        config.obstacle_first_distance,
        config.obstacle_min_spacing,
        config.obstacle_max_spacing,
        config.collision_half_depth,
        config.collision_half_lane,
        config.chasm_action_distance,
        config.chasm_plate_duration,
        config.chasm_plate_stop_offset,
        config.chasm_exit_offset
    };
}

bool valid_config(const RunnerConfig& config) noexcept {
    for (const float value : config_values(config)) {
        if (!std::isfinite(value)) return false;
    }
    return config.base_speed > 0.0F &&
        config.max_speed >= config.base_speed &&
        config.speed_ramp_delay_seconds >= 0.0F &&
        config.speed_ramp_per_second >= 0.0F &&
        config.lane_change_speed > 0.0F &&
        config.obstacle_min_spacing >= 4.0F &&
        config.obstacle_max_spacing >= config.obstacle_min_spacing &&
        config.collision_half_depth >= 0.05F &&
        config.collision_half_lane >= 0.05F &&
        config.chasm_action_distance > config.collision_half_depth &&
        config.chasm_plate_duration >= 0.2F &&
        config.chasm_plate_stop_offset > 0.0F &&
        config.chasm_exit_offset >= 1.5F;
}

bool has_input(const RunnerInput& input) noexcept {
    return input.lane_delta != 0 || input.start || input.restart || input.action;
}

bool valid_input(const RunnerInput& input) noexcept {
    return input.lane_delta >= -1 && input.lane_delta <= 1;
}

bool validate_replay(const RunnerReplay& replay, std::string* error) {
    if (replay.format_version != kRunnerReplayFormatVersion) {
        set_error(error, "Unsupported replay format version.");
        return false;
    }
    if (replay.ruleset_version != kRunnerReplayRulesetVersion) {
        set_error(error, "Unsupported simulation ruleset version.");
        return false;
    }
    if (!std::isfinite(replay.fixed_delta_seconds) ||
        replay.fixed_delta_seconds <= 0.0F ||
        replay.fixed_delta_seconds > kMaximumReplayDelta) {
        set_error(error, "Replay fixed delta is invalid.");
        return false;
    }
    if (!valid_config(replay.config)) {
        set_error(error, "Replay configuration contains invalid values.");
        return false;
    }

    std::uint64_t previous_event_frame = 0;
    bool first_event = true;
    for (const auto& event : replay.events) {
        if (event.frame_index >= replay.frame_count ||
            !valid_input(event.input) ||
            !has_input(event.input) ||
            (!first_event && event.frame_index <= previous_event_frame)) {
            set_error(error, "Replay events must be non-empty, ordered, and inside the frame range.");
            return false;
        }
        previous_event_frame = event.frame_index;
        first_event = false;
    }

    std::uint64_t previous_checkpoint_frame = 0;
    bool first_checkpoint = true;
    for (const auto& checkpoint : replay.checkpoints) {
        if (checkpoint.frame_count > replay.frame_count ||
            (!first_checkpoint && checkpoint.frame_count <= previous_checkpoint_frame)) {
            set_error(error, "Replay checkpoints must be ordered and inside the frame range.");
            return false;
        }
        previous_checkpoint_frame = checkpoint.frame_count;
        first_checkpoint = false;
    }
    return true;
}

bool read_tag(std::istream& input, const char* expected, std::string* error) {
    std::string tag;
    if (!(input >> tag) || tag != expected) {
        set_error(error, "Replay file has an invalid or truncated section.");
        return false;
    }
    return true;
}

} // namespace

RunnerReplayRecorder::RunnerReplayRecorder(
    std::uint64_t seed,
    RunnerConfig config,
    float fixed_delta_seconds,
    std::uint64_t checkpoint_interval) {
    replay_.seed = seed;
    replay_.config = config;
    replay_.fixed_delta_seconds = fixed_delta_seconds;
    checkpoint_interval_ = checkpoint_interval == 0 ? 60 : checkpoint_interval;
}

void RunnerReplayRecorder::step(RunnerGame& game, const RunnerInput& input) {
    if (has_input(input)) {
        replay_.events.push_back({replay_.frame_count, input});
    }

    game.step(input, replay_.fixed_delta_seconds);
    ++replay_.frame_count;

    if (replay_.frame_count % checkpoint_interval_ == 0) {
        replay_.checkpoints.push_back({
            replay_.frame_count,
            game.deterministic_hash()
        });
    }
}

void RunnerReplayRecorder::finish(const RunnerGame& game) {
    if (replay_.checkpoints.empty() ||
        replay_.checkpoints.back().frame_count != replay_.frame_count) {
        replay_.checkpoints.push_back({
            replay_.frame_count,
            game.deterministic_hash()
        });
    }
}

bool write_runner_replay(
    std::ostream& output,
    const RunnerReplay& replay,
    std::string* error) {
    if (!validate_replay(replay, error)) return false;

    output.imbue(std::locale::classic());
    output << "XNO_RUNNER_REPLAY " << replay.format_version << '\n';
    output << "ruleset " << replay.ruleset_version << '\n';
    output << "seed " << replay.seed << '\n';
    output << "fixed_delta " << std::setprecision(std::numeric_limits<float>::max_digits10)
           << replay.fixed_delta_seconds << '\n';

    const auto values = config_values(replay.config);
    output << "config";
    for (const float value : values) output << ' ' << value;
    output << '\n';

    output << "frames " << replay.frame_count << '\n';
    output << "events " << replay.events.size() << '\n';
    for (const auto& event : replay.events) {
        output << "event " << event.frame_index
               << ' ' << event.input.lane_delta
               << ' ' << static_cast<int>(event.input.start)
               << ' ' << static_cast<int>(event.input.restart)
               << ' ' << static_cast<int>(event.input.action) << '\n';
    }

    output << "checkpoints " << replay.checkpoints.size() << '\n';
    for (const auto& checkpoint : replay.checkpoints) {
        output << "checkpoint " << checkpoint.frame_count
               << ' ' << checkpoint.state_hash << '\n';
    }
    output << "END\n";

    if (!output.good()) {
        set_error(error, "Could not write replay data.");
        return false;
    }
    return true;
}

bool read_runner_replay(
    std::istream& input,
    RunnerReplay& replay,
    std::string* error) {
    input.imbue(std::locale::classic());

    RunnerReplay parsed{};
    std::string magic;
    if (!(input >> magic >> parsed.format_version) || magic != "XNO_RUNNER_REPLAY") {
        set_error(error, "Not an XNO Runner replay file.");
        return false;
    }

    if (!read_tag(input, "ruleset", error) ||
        !(input >> parsed.ruleset_version) ||
        !read_tag(input, "seed", error) ||
        !(input >> parsed.seed) ||
        !read_tag(input, "fixed_delta", error) ||
        !(input >> parsed.fixed_delta_seconds) ||
        !read_tag(input, "config", error)) {
        if (error != nullptr && error->empty()) *error = "Replay header is truncated.";
        return false;
    }

    auto values = config_values(parsed.config);
    for (float& value : values) {
        if (!(input >> value)) {
            set_error(error, "Replay configuration is truncated.");
            return false;
        }
    }
    parsed.config.base_speed = values[0];
    parsed.config.max_speed = values[1];
    parsed.config.speed_ramp_delay_seconds = values[2];
    parsed.config.speed_ramp_per_second = values[3];
    parsed.config.lane_change_speed = values[4];
    parsed.config.obstacle_first_distance = values[5];
    parsed.config.obstacle_min_spacing = values[6];
    parsed.config.obstacle_max_spacing = values[7];
    parsed.config.collision_half_depth = values[8];
    parsed.config.collision_half_lane = values[9];
    parsed.config.chasm_action_distance = values[10];
    parsed.config.chasm_plate_duration = values[11];
    parsed.config.chasm_plate_stop_offset = values[12];
    parsed.config.chasm_exit_offset = values[13];

    std::uint64_t event_count = 0;
    if (!read_tag(input, "frames", error) ||
        !(input >> parsed.frame_count) ||
        !read_tag(input, "events", error) ||
        !(input >> event_count) ||
        event_count > parsed.frame_count) {
        set_error(error, "Replay frame or event count is invalid.");
        return false;
    }

    parsed.events.reserve(static_cast<std::size_t>(event_count));
    for (std::uint64_t index = 0; index < event_count; ++index) {
        RunnerReplayEvent event{};
        int start = 0;
        int restart = 0;
        int action = 0;
        if (!read_tag(input, "event", error) ||
            !(input >> event.frame_index
                    >> event.input.lane_delta
                    >> start >> restart >> action) ||
            (start != 0 && start != 1) ||
            (restart != 0 && restart != 1) ||
            (action != 0 && action != 1)) {
            set_error(error, "Replay input event is malformed.");
            return false;
        }
        event.input.start = start != 0;
        event.input.restart = restart != 0;
        event.input.action = action != 0;
        parsed.events.push_back(event);
    }

    std::uint64_t checkpoint_count = 0;
    if (!read_tag(input, "checkpoints", error) ||
        !(input >> checkpoint_count) ||
        checkpoint_count > parsed.frame_count + 1U) {
        set_error(error, "Replay checkpoint count is invalid.");
        return false;
    }

    parsed.checkpoints.reserve(static_cast<std::size_t>(checkpoint_count));
    for (std::uint64_t index = 0; index < checkpoint_count; ++index) {
        RunnerReplayCheckpoint checkpoint{};
        if (!read_tag(input, "checkpoint", error) ||
            !(input >> checkpoint.frame_count >> checkpoint.state_hash)) {
            set_error(error, "Replay checkpoint is malformed.");
            return false;
        }
        parsed.checkpoints.push_back(checkpoint);
    }

    if (!read_tag(input, "END", error)) return false;
    std::string trailing;
    if (input >> trailing) {
        set_error(error, "Replay file contains unexpected trailing data.");
        return false;
    }
    if (!validate_replay(parsed, error)) return false;

    replay = std::move(parsed);
    return true;
}

RunnerReplayVerification verify_runner_replay(const RunnerReplay& replay) {
    RunnerReplayVerification result{};
    if (!validate_replay(replay, &result.error)) return result;

    RunnerGame game(replay.seed, replay.config);
    std::size_t event_index = 0;
    std::size_t checkpoint_index = 0;

    if (!replay.checkpoints.empty() &&
        replay.checkpoints.front().frame_count == 0) {
        const auto actual = game.deterministic_hash();
        const auto expected = replay.checkpoints.front().state_hash;
        if (actual != expected) {
            result.mismatch_frame_count = 0;
            result.expected_hash = expected;
            result.actual_hash = actual;
            result.error = "Initial state does not match replay checkpoint.";
            return result;
        }
        ++checkpoint_index;
    }

    for (std::uint64_t frame = 0; frame < replay.frame_count; ++frame) {
        RunnerInput input{};
        if (event_index < replay.events.size() &&
            replay.events[event_index].frame_index == frame) {
            input = replay.events[event_index].input;
            ++event_index;
        }

        game.step(input, replay.fixed_delta_seconds);
        result.frames_processed = frame + 1U;

        if (checkpoint_index < replay.checkpoints.size() &&
            replay.checkpoints[checkpoint_index].frame_count == result.frames_processed) {
            const std::uint64_t expected = replay.checkpoints[checkpoint_index].state_hash;
            const std::uint64_t actual = game.deterministic_hash();
            if (actual != expected) {
                result.mismatch_frame_count = result.frames_processed;
                result.expected_hash = expected;
                result.actual_hash = actual;
                result.error = "Deterministic state mismatch at checkpoint.";
                return result;
            }
            ++checkpoint_index;
        }
    }

    if (event_index != replay.events.size() ||
        checkpoint_index != replay.checkpoints.size()) {
        result.error = "Replay contains events or checkpoints outside the playback range.";
        return result;
    }

    result.success = true;
    return result;
}

} // namespace xno::runner
