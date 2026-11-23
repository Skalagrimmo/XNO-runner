#include "runner/core/runner_game.hpp"

#include <algorithm>
#include <bit>
#include <cmath>

namespace xno::runner {
namespace {
constexpr float kMaxStepSeconds = 0.1F;
constexpr float kScorePerMeter = 10.0F;
constexpr std::uint64_t kObstacleClearBonus = 250;
constexpr float kObstacleRetireDistance = 1.0F;
constexpr float kSpawnLookAhead = 64.0F;
constexpr float kCrashEffectDuration = 1.2F;

float approach(float current, float target, float amount) noexcept {
    if (current < target) return std::min(current + amount, target);
    return std::max(current - amount, target);
}

std::uint64_t calculate_score(const RunnerState& state) noexcept {
    const auto distance_score = static_cast<std::uint64_t>(
        std::max(0.0F, state.distance) * kScorePerMeter);
    return distance_score + state.obstacles_cleared * kObstacleClearBonus;
}
} // namespace

RunnerGame::RunnerGame(std::uint64_t seed, RunnerConfig config)
    : config_(config) {
    config_.base_speed = std::max(config_.base_speed, 0.1F);
    config_.max_speed = std::max(config_.max_speed, config_.base_speed);
    config_.speed_ramp_per_second = std::max(config_.speed_ramp_per_second, 0.0F);
    config_.lane_change_speed = std::max(config_.lane_change_speed, 0.1F);
    config_.obstacle_min_spacing = std::max(config_.obstacle_min_spacing, 4.0F);
    config_.obstacle_max_spacing = std::max(
        config_.obstacle_max_spacing, config_.obstacle_min_spacing);
    config_.collision_half_depth = std::max(config_.collision_half_depth, 0.05F);
    config_.collision_half_lane = std::max(config_.collision_half_lane, 0.05F);
    config_.chasm_action_distance = std::max(
        config_.chasm_action_distance, config_.collision_half_depth + 0.1F);
    config_.chasm_plate_duration = std::max(config_.chasm_plate_duration, 0.2F);
    config_.chasm_plate_stop_offset = std::max(config_.chasm_plate_stop_offset, 0.1F);
    config_.chasm_exit_offset = std::max(config_.chasm_exit_offset, 1.5F);
    reset(seed);
}

void RunnerGame::reset(std::uint64_t seed) {
    seed_ = seed;
    random_state_ = seed == 0 ? 0x9E3779B97F4A7C15ULL : seed;
    next_obstacle_id_ = 1;
    obstacles_since_chasm_ = 2;
    next_spawn_distance_ = config_.obstacle_first_distance;
    state_ = RunnerState{};
    state_.speed = config_.base_speed;
    obstacles_.clear();
    spawn_ahead();
}

void RunnerGame::start() noexcept {
    if (state_.status == RunStatus::ready) {
        state_.status = RunStatus::running;
    }
}

std::uint64_t RunnerGame::next_random() noexcept {
    // SplitMix64: compact, deterministic, and independent of standard-library RNGs.
    std::uint64_t z = (random_state_ += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30U)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27U)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31U);
}

void RunnerGame::spawn_ahead() {
    const float spawn_limit = state_.distance + kSpawnLookAhead;
    while (next_spawn_distance_ <= spawn_limit) {
        const float spacing_range = config_.obstacle_max_spacing -
            config_.obstacle_min_spacing;
        const float unit = static_cast<float>(next_random() & 0xFFFFFFULL) /
            static_cast<float>(0x1000000ULL);
        const float spacing = config_.obstacle_min_spacing + unit * spacing_range;
        const int lane = static_cast<int>(next_random() % 3ULL);
        const std::uint64_t hazard_roll = next_random() % 100ULL;
        const std::uint64_t obstacle_id = next_obstacle_id_++;

        // Keep the opening obstacle readable, then let BIT_CHASM appear
        // probabilistically. Two FOLD_GATE obstacles must separate chasms.
        constexpr std::uint64_t kBitChasmChancePercent = 28ULL;
        const bool chasm_allowed = obstacle_id > 1ULL &&
            obstacles_since_chasm_ >= 2U;
        const bool spawn_chasm = chasm_allowed &&
            hazard_roll < kBitChasmChancePercent;
        const HazardType type = spawn_chasm
            ? HazardType::bit_chasm
            : HazardType::fold_gate;

        if (spawn_chasm) {
            obstacles_since_chasm_ = 0U;
        } else if (obstacles_since_chasm_ < 3U) {
            ++obstacles_since_chasm_;
        }
        obstacles_.push_back({
            obstacle_id,
            next_spawn_distance_,
            lane,
            type,
            false
        });
        next_spawn_distance_ += spacing;
    }
}

void RunnerGame::update_lane(float delta_seconds) noexcept {
    const float target = static_cast<float>(state_.target_lane - 1);
    state_.lane_position = approach(
        state_.lane_position,
        target,
        config_.lane_change_speed * delta_seconds);
}

void RunnerGame::update_obstacles(float previous_distance) noexcept {
    for (auto& obstacle : obstacles_) {
        if (obstacle.resolved) continue;

        const float lower = obstacle.distance - config_.collision_half_depth;
        const float upper = obstacle.distance + config_.collision_half_depth;
        const bool crossed = state_.distance >= lower && previous_distance <= upper;
        if (!crossed) continue;

        const float blocked_lane_position = static_cast<float>(obstacle.blocked_lane - 1);
        const float lateral_error = std::abs(state_.lane_position - blocked_lane_position);
        obstacle.resolved = true;

        // BIT_CHASM spans the entire corridor. It can only be crossed by its
        // two-stage contextual action; changing lanes is deliberately not enough.
        const bool hit = obstacle.type == HazardType::bit_chasm ||
            lateral_error <= config_.collision_half_lane;
        if (hit) {
            state_.status = RunStatus::crashed;
            state_.collision_obstacle_id = obstacle.id;
            state_.crash_effect_elapsed = 0.0F;
            state_.speed = 0.0F;
            return;
        }

        ++state_.obstacles_cleared;
        state_.score += kObstacleClearBonus;
    }

    obstacles_.erase(
        std::remove_if(
            obstacles_.begin(),
            obstacles_.end(),
            [this](const RunnerObstacle& obstacle) {
                return obstacle.resolved &&
                    obstacle.distance + kObstacleRetireDistance < state_.distance;
            }),
        obstacles_.end());
}

std::uint64_t RunnerGame::deterministic_hash() const noexcept {
    static_assert(sizeof(float) == sizeof(std::uint32_t));
    constexpr std::uint64_t kFnvOffset = 14695981039346656037ULL;
    constexpr std::uint64_t kFnvPrime = 1099511628211ULL;
    std::uint64_t hash = kFnvOffset;

    const auto append_u64 = [&hash](std::uint64_t value) {
        constexpr std::uint64_t prime = kFnvPrime;
        for (unsigned int byte = 0; byte < 8U; ++byte) {
            hash ^= (value >> (byte * 8U)) & 0xFFULL;
            hash *= prime;
        }
    };
    const auto append_float = [&append_u64](float value) {
        append_u64(std::bit_cast<std::uint32_t>(value));
    };
    const auto append_bool = [&append_u64](bool value) {
        append_u64(value ? 1ULL : 0ULL);
    };

    // Include all configuration and generator internals, not only visible HUD
    // state. A matching hash therefore checks the future obstacle stream too.
    append_float(config_.base_speed);
    append_float(config_.max_speed);
    append_float(config_.speed_ramp_per_second);
    append_float(config_.lane_change_speed);
    append_float(config_.obstacle_first_distance);
    append_float(config_.obstacle_min_spacing);
    append_float(config_.obstacle_max_spacing);
    append_float(config_.collision_half_depth);
    append_float(config_.collision_half_lane);
    append_float(config_.chasm_action_distance);
    append_float(config_.chasm_plate_duration);
    append_float(config_.chasm_plate_stop_offset);
    append_float(config_.chasm_exit_offset);

    append_u64(seed_);
    append_u64(random_state_);
    append_u64(next_obstacle_id_);
    append_u64(obstacles_since_chasm_);
    append_float(next_spawn_distance_);

    append_u64(static_cast<std::uint64_t>(state_.status));
    append_u64(static_cast<std::uint64_t>(static_cast<std::int64_t>(state_.target_lane)));
    append_float(state_.lane_position);
    append_float(state_.distance);
    append_float(state_.elapsed_seconds);
    append_float(state_.speed);
    append_float(state_.crash_effect_elapsed);
    append_float(state_.chasm_plate_elapsed);
    append_u64(state_.score);
    append_u64(state_.simulation_ticks);
    append_u64(state_.obstacles_cleared);
    append_u64(state_.collision_obstacle_id);
    append_u64(state_.active_chasm_obstacle_id);
    append_u64(static_cast<std::uint64_t>(static_cast<std::int64_t>(state_.active_chasm_lane)));
    append_bool(state_.on_chasm_plate);

    append_u64(static_cast<std::uint64_t>(obstacles_.size()));
    for (const auto& obstacle : obstacles_) {
        append_u64(obstacle.id);
        append_float(obstacle.distance);
        append_u64(static_cast<std::uint64_t>(
            static_cast<std::int64_t>(obstacle.blocked_lane)));
        append_u64(static_cast<std::uint64_t>(obstacle.type));
        append_bool(obstacle.resolved);
    }
    return hash;
}

bool RunnerGame::action_available() const noexcept {
    if (state_.status != RunStatus::running) return false;
    if (state_.on_chasm_plate) return true;

    for (const auto& obstacle : obstacles_) {
        if (obstacle.resolved) continue;
        if (obstacle.type != HazardType::bit_chasm) return false;

        const float approach_distance = obstacle.distance - state_.distance;
        return approach_distance <= config_.chasm_action_distance &&
            approach_distance > config_.collision_half_depth;
    }
    return false;
}

void RunnerGame::step(const RunnerInput& input, float delta_seconds) {
    if (input.restart && state_.status == RunStatus::crashed) {
        reset(seed_);
        start();
        return;
    }
    if (input.start && state_.status == RunStatus::ready) {
        start();
    }

    const float dt = std::clamp(delta_seconds, 0.0F, kMaxStepSeconds);
    if (dt <= 0.0F) return;

    // The presentation timer continues after a crash; world simulation stays frozen.
    if (state_.status == RunStatus::crashed) {
        state_.crash_effect_elapsed = std::min(
            state_.crash_effect_elapsed + dt, kCrashEffectDuration);
        return;
    }
    if (state_.status != RunStatus::running) return;

    // The player is physically on the anchored tile. World movement and score
    // pause, but the tile's collapse timer continues at the fixed simulation rate.
    if (state_.on_chasm_plate) {
        if (input.action) {
            const auto active = std::find_if(
                obstacles_.begin(),
                obstacles_.end(),
                [this](const RunnerObstacle& obstacle) {
                    return obstacle.id == state_.active_chasm_obstacle_id &&
                        !obstacle.resolved &&
                        obstacle.type == HazardType::bit_chasm;
                });
            if (active != obstacles_.end() &&
                state_.chasm_plate_elapsed < config_.chasm_plate_duration) {
                active->resolved = true;
                state_.on_chasm_plate = false;
                state_.active_chasm_obstacle_id = 0;
                state_.chasm_plate_elapsed = 0.0F;
                state_.distance = active->distance + config_.chasm_exit_offset;
                state_.speed = std::min(
                    config_.base_speed + state_.elapsed_seconds * config_.speed_ramp_per_second,
                    config_.max_speed);
                ++state_.obstacles_cleared;
                state_.elapsed_seconds += dt;
                ++state_.simulation_ticks;
                spawn_ahead();
                state_.score = calculate_score(state_);
                return;
            }
        }

        state_.chasm_plate_elapsed = std::min(
            state_.chasm_plate_elapsed + dt, config_.chasm_plate_duration);
        ++state_.simulation_ticks;
        if (state_.chasm_plate_elapsed >= config_.chasm_plate_duration) {
            state_.status = RunStatus::crashed;
            state_.collision_obstacle_id = state_.active_chasm_obstacle_id;
            state_.crash_effect_elapsed = 0.0F;
            state_.on_chasm_plate = false;
            state_.speed = 0.0F;
        }
        return;
    }

    // First press: anchor a plate in the player's current lane and blink onto it.
    // Only the next unresolved obstacle may be activated, and only in its approach
    // window; pressing early or after the edge has no effect.
    if (input.action) {
        for (const auto& obstacle : obstacles_) {
            if (obstacle.resolved) continue;
            if (obstacle.type != HazardType::bit_chasm) break;

            const float approach_distance = obstacle.distance - state_.distance;
            if (approach_distance <= config_.chasm_action_distance &&
                approach_distance > config_.collision_half_depth) {
                state_.on_chasm_plate = true;
                state_.active_chasm_obstacle_id = obstacle.id;
                state_.active_chasm_lane = state_.target_lane;
                state_.lane_position = static_cast<float>(state_.target_lane - 1);
                state_.distance = obstacle.distance - config_.chasm_plate_stop_offset;
                state_.speed = 0.0F;
                state_.chasm_plate_elapsed = 0.0F;
            }
            break;
        }
        if (state_.on_chasm_plate) return;
    }

    if (input.lane_delta < 0) {
        state_.target_lane = std::max(0, state_.target_lane - 1);
    } else if (input.lane_delta > 0) {
        state_.target_lane = std::min(2, state_.target_lane + 1);
    }

    const float previous_distance = state_.distance;
    state_.speed = std::min(
        config_.base_speed + state_.elapsed_seconds * config_.speed_ramp_per_second,
        config_.max_speed);

    update_lane(dt);
    state_.distance += state_.speed * dt;
    state_.elapsed_seconds += dt;
    ++state_.simulation_ticks;
    spawn_ahead();
    update_obstacles(previous_distance);
    state_.score = calculate_score(state_);
}

} // namespace xno::runner
