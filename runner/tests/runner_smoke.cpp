#include "runner/core/runner_game.hpp"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>

using xno::runner::RunStatus;
using xno::runner::RunnerGame;
using xno::runner::RunnerInput;

namespace {
void step_many(RunnerGame& game, int count, RunnerInput input = {}) {
    for (int i = 0; i < count; ++i) game.step(input, 1.0F / 60.0F);
}
}

int main() {
    // The same seed must produce the same upcoming obstacle sequence.
    RunnerGame first(0x12345678ULL);
    RunnerGame second(0x12345678ULL);
    assert(first.obstacles().size() == second.obstacles().size());
    for (std::size_t i = 0; i < first.obstacles().size(); ++i) {
        assert(first.obstacles()[i].distance == second.obstacles()[i].distance);
        assert(first.obstacles()[i].blocked_lane == second.obstacles()[i].blocked_lane);
    }

    // The run is inert until started, then advances on fixed simulation steps.
    assert(first.state().status == RunStatus::ready);
    step_many(first, 120);
    assert(first.state().distance == 0.0F);
    first.step({.start = true}, 1.0F / 60.0F);
    assert(first.state().status == RunStatus::running);
    step_many(first, 60);
    assert(first.state().distance > 0.0F);
    assert(first.state().simulation_ticks == 61);

    // Move to a lane known to be clear and verify that the first obstacle is passed.
    RunnerGame safe(0xA11CEULL);
    const auto first_obstacle = safe.obstacles().front();
    safe.start();
    int safe_delta = 0;
    if (first_obstacle.blocked_lane == 1) safe_delta = -1;
    // If the center lane is not blocked, remain in the center.
    safe.step({.lane_delta = safe_delta}, 1.0F / 60.0F);
    const int travel_steps = static_cast<int>(
        std::ceil((first_obstacle.distance + 1.5F) / 8.0F * 60.0F));
    step_many(safe, travel_steps);
    assert(safe.state().status == RunStatus::running);
    assert(safe.state().obstacles_cleared >= 1);

    // Lane requests clamp at the left/right boundaries.
    RunnerGame boundaries(0xFACEULL);
    boundaries.start();
    boundaries.step({.lane_delta = -1}, 1.0F / 60.0F);
    step_many(boundaries, 30);
    boundaries.step({.lane_delta = -1}, 1.0F / 60.0F);
    assert(boundaries.state().target_lane == 0);
    boundaries.step({.lane_delta = +1}, 1.0F / 60.0F);
    assert(boundaries.state().target_lane == 1);

    // Sprint changes speed, but does not change the deterministic obstacle stream.
    RunnerGame sprinter(0x515052494E54ULL);
    sprinter.start();
    sprinter.step({.sprint = true}, 1.0F / 60.0F);
    assert(sprinter.state().sprinting);
    assert(sprinter.state().speed > 8.0F);
    assert(sprinter.state().distance > 8.0F / 60.0F);

    // Deliberately enter the first obstacle's lane early and verify collision.
    RunnerGame crash(0xBADC0FFEEULL);
    const auto crash_obstacle = crash.obstacles().front();
    crash.start();
    int crash_delta = 0;
    if (crash_obstacle.blocked_lane == 0) crash_delta = -1;
    if (crash_obstacle.blocked_lane == 2) crash_delta = +1;
    if (crash_delta != 0) {
        crash.step({.lane_delta = crash_delta}, 1.0F / 60.0F);
    }
    const int crash_steps = static_cast<int>(
        std::ceil((crash_obstacle.distance + 1.0F) / 8.0F * 60.0F));
    step_many(crash, crash_steps);
    assert(crash.state().status == RunStatus::crashed);
    assert(crash.state().collision_obstacle_id == crash_obstacle.id);
    const auto expected_crash_score =
        static_cast<std::uint64_t>(crash.state().distance * 10.0F) +
        crash.state().obstacles_cleared * 250ULL;
    assert(crash.state().score == expected_crash_score);

    // A crashed run can be restarted with the same deterministic seed.
    crash.step({.restart = true}, 1.0F / 60.0F);
    assert(crash.state().status == RunStatus::running);
    assert(crash.state().distance == 0.0F);
    assert(crash.obstacles().front().blocked_lane == crash_obstacle.blocked_lane);

    std::cout << "XNO Runner core smoke tests passed.\n";
    return 0;
}
