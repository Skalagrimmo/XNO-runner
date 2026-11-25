#pragma once

#include <cstdint>
#include <vector>

namespace xno::runner {

enum class RunStatus : std::uint8_t {
    ready,
    running,
    crashed
};

enum class HazardType : std::uint8_t {
    fold_gate,
    bit_chasm,
    raster_sweeper
};

struct RunnerObstacle {
    std::uint64_t id = 0;
    float distance = 0.0F;
    int blocked_lane = 1; // FOLD_GATE only: 0 = left, 1 = centre, 2 = right
    HazardType type = HazardType::fold_gate;
    bool resolved = false;
    int sweep_direction = 1; // RASTER_SWEEPER: +1 left-to-right, -1 right-to-left
    float sweep_phase_offset = 0.0F;
};

struct RunnerInput {
    int lane_delta = 0; // -1 or +1; applied as a discrete lane request
    bool start = false;
    bool restart = false;
    bool action = false; // edge-triggered contextual action
};

struct RunnerConfig {
    float base_speed = 8.0F;
    float max_speed = 15.0F;
    float speed_ramp_delay_seconds = 12.0F;
    float speed_ramp_per_second = 0.065F;
    float lane_change_speed = 5.0F; // lane widths per second
    float obstacle_first_distance = 34.0F;
    float obstacle_min_spacing = 28.0F;
    float obstacle_max_spacing = 40.0F;
    float collision_half_depth = 0.55F;
    float collision_half_lane = 0.36F;
    float chasm_action_distance = 7.0F;
    float chasm_plate_duration = 1.15F;
    float chasm_plate_stop_offset = 0.90F;
    float chasm_exit_offset = 2.80F;
};

struct RunnerState {
    RunStatus status = RunStatus::ready;
    int target_lane = 1;
    float lane_position = 0.0F; // -1 = left, 0 = centre, +1 = right
    float distance = 0.0F;
    float elapsed_seconds = 0.0F;
    float speed = 0.0F;
    float crash_effect_elapsed = 0.0F;
    float chasm_plate_elapsed = 0.0F;
    std::uint64_t score = 0;
    std::uint64_t simulation_ticks = 0;
    std::uint64_t obstacles_cleared = 0;
    std::uint64_t collision_obstacle_id = 0;
    std::uint64_t active_chasm_obstacle_id = 0;
    int active_chasm_lane = 1;
    bool on_chasm_plate = false;
};

class RunnerGame final {
public:
    explicit RunnerGame(std::uint64_t seed = 1, RunnerConfig config = {});

    void reset(std::uint64_t seed);
    void start() noexcept;
    void step(const RunnerInput& input, float delta_seconds);

    [[nodiscard]] const RunnerState& state() const noexcept { return state_; }
    [[nodiscard]] const std::vector<RunnerObstacle>& obstacles() const noexcept {
        return obstacles_;
    }
    [[nodiscard]] std::uint64_t seed() const noexcept { return seed_; }
    [[nodiscard]] const RunnerConfig& config() const noexcept { return config_; }
    [[nodiscard]] std::uint64_t deterministic_hash() const noexcept;
    [[nodiscard]] bool action_available() const noexcept;
    [[nodiscard]] static int raster_sweeper_lane(
        const RunnerObstacle& obstacle,
        float elapsed_seconds) noexcept;

private:
    [[nodiscard]] std::uint64_t next_random() noexcept;
    void spawn_ahead();
    void update_lane(float delta_seconds) noexcept;
    void update_obstacles(float previous_distance) noexcept;

    RunnerConfig config_{};
    RunnerState state_{};
    std::uint64_t seed_ = 1;
    std::uint64_t random_state_ = 1;
    std::uint64_t next_obstacle_id_ = 1;
    std::uint32_t obstacles_since_chasm_ = 2;
    float next_spawn_distance_ = 0.0F;
    std::vector<RunnerObstacle> obstacles_;
};

} // namespace xno::runner
