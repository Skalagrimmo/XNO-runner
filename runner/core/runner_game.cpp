#include "runner/core/runner_game.hpp"

#include <algorithm>
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
} // namespace

RunnerGame::RunnerGame(std::uint64_t seed, RunnerConfig config)
    : config_(config) {
    config_.base_speed = std::max(config_.base_speed, 0.1F);
    config_.max_speed = std::max(config_.max_speed, config_.base_speed);
    config_.speed_ramp_per_second = std::max(config_.speed_ramp_per_second, 0.0F);
    config_.sprint_multiplier = std::max(config_.sprint_multiplier, 1.0F);
    config_.lane_change_speed = std::max(config_.lane_change_speed, 0.1F);
    config_.obstacle_min_spacing = std::max(config_.obstacle_min_spacing, 4.0F);
    config_.obstacle_max_spacing = std::max(
        config_.obstacle_max_spacing, config_.obstacle_min_spacing);
    config_.collision_half_depth = std::max(config_.collision_half_depth, 0.05F);
    config_.collision_half_lane = std::max(config_.collision_half_lane, 0.05F);
    reset(seed);
}

void RunnerGame::reset(std::uint64_t seed) {
    seed_ = seed;
    random_state_ = seed == 0 ? 0x9E3779B97F4A7C15ULL : seed;
    next_obstacle_id_ = 1;
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
        const std::uint64_t obstacle_id = next_obstacle_id_++;
        // Alternate the two first-pass silhouettes without consuming RNG state.
        const HazardType type = (obstacle_id % 2ULL == 0ULL)
            ? HazardType::bit_chasm
            : HazardType::fold_gate;
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

        if (lateral_error <= config_.collision_half_lane) {
            state_.status = RunStatus::crashed;
            state_.collision_obstacle_id = obstacle.id;
            state_.crash_effect_elapsed = 0.0F;
            state_.sprinting = false;
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

    if (input.lane_delta < 0) {
        state_.target_lane = std::max(0, state_.target_lane - 1);
    } else if (input.lane_delta > 0) {
        state_.target_lane = std::min(2, state_.target_lane + 1);
    }

    const float previous_distance = state_.distance;
    state_.sprinting = input.sprint;
    const float ramped_speed = std::min(
        config_.base_speed + state_.elapsed_seconds * config_.speed_ramp_per_second,
        config_.max_speed);
    state_.speed = ramped_speed * (state_.sprinting ? config_.sprint_multiplier : 1.0F);

    update_lane(dt);
    state_.distance += state_.speed * dt;
    state_.elapsed_seconds += dt;
    ++state_.simulation_ticks;
    spawn_ahead();
    update_obstacles(previous_distance);

    const auto distance_score = static_cast<std::uint64_t>(
        std::max(0.0F, state_.distance) * kScorePerMeter);
    state_.score = distance_score +
        state_.obstacles_cleared * kObstacleClearBonus;
}

} // namespace xno::runner
