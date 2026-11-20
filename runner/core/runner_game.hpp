#pragma once

#include <cstdint>
#include <vector>

namespace xno::runner {

enum class RunStatus : std::uint8_t {
    ready,
    running,
    crashed
};

struct RunnerObstacle {
    std::uint64_t id = 0;
    float distance = 0.0F;
    int blocked_lane = 1; // 0 = left, 1 = centre, 2 = right
    bool resolved = false;
};

struct RunnerInput {
    int lane_delta = 0; // -1 or +1; applied as a discrete lane request
    bool start = false;
    bool restart = false;
    bool sprint = false;
};

struct RunnerConfig {
    float base_speed = 8.0F;
    float max_speed = 15.0F;
    float speed_ramp_per_second = 0.12F;
    float sprint_multiplier = 1.25F;
    float lane_change_speed = 5.0F; // lane widths per second
    float obstacle_first_distance = 24.0F;
    float obstacle_min_spacing = 18.0F;
    float obstacle_max_spacing = 24.0F;
    float collision_half_depth = 0.55F;
    float collision_half_lane = 0.36F;
};

struct RunnerState {
    RunStatus status = RunStatus::ready;
    int target_lane = 1;
    float lane_position = 0.0F; // -1 = left, 0 = centre, +1 = right
    float distance = 0.0F;
    float elapsed_seconds = 0.0F;
    float speed = 0.0F;
    std::uint64_t score = 0;
    std::uint64_t simulation_ticks = 0;
    std::uint64_t obstacles_cleared = 0;
    std::uint64_t collision_obstacle_id = 0;
    bool sprinting = false;
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
    float next_spawn_distance_ = 0.0F;
    std::vector<RunnerObstacle> obstacles_;
};

} // namespace xno::runner
