#include "runner/core/runner_game.hpp"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>

using xno::runner::HazardType;
using xno::runner::RunStatus;
using xno::runner::RunnerGame;
using xno::runner::RunnerInput;

namespace {
constexpr float kStep = 1.0F / 60.0F;

void step_many(RunnerGame& game, int count, RunnerInput input = {}) {
    for (int i = 0; i < count; ++i) game.step(input, kStep);
}

void reach_first_chasm(RunnerGame& game, std::uint64_t& chasm_id) {
    const auto first_obstacle = game.obstacles().front();
    assert(first_obstacle.type == HazardType::fold_gate);
    assert(game.obstacles().size() >= 2);
    const auto chasm = game.obstacles()[1];
    assert(chasm.type == HazardType::bit_chasm);
    chasm_id = chasm.id;

    game.start();
    // Keep the player out of the FOLD_GATE lane; BIT_CHASM itself spans all lanes.
    const int safe_delta = first_obstacle.blocked_lane == 1 ? -1 : 0;
    if (safe_delta != 0) game.step({.lane_delta = safe_delta}, kStep);

    const float target_distance = chasm.distance -
        game.config().chasm_action_distance + 0.1F;
    for (int step = 0;
         step < 1200 && game.state().distance < target_distance;
         ++step) {
        game.step({}, kStep);
        assert(game.state().status == RunStatus::running);
    }
    assert(game.action_available());
}
}

int main() {
    // The same seed must produce the same upcoming obstacle sequence and silhouettes.
    RunnerGame first(0x12345678ULL);
    RunnerGame second(0x12345678ULL);
    assert(first.obstacles().size() == second.obstacles().size());
    for (std::size_t i = 0; i < first.obstacles().size(); ++i) {
        assert(first.obstacles()[i].distance == second.obstacles()[i].distance);
        assert(first.obstacles()[i].blocked_lane == second.obstacles()[i].blocked_lane);
        assert(first.obstacles()[i].type == second.obstacles()[i].type);
    }
    assert(first.obstacles().size() >= 2);
    assert(first.obstacles()[0].type == HazardType::fold_gate);
    assert(first.obstacles()[1].type == HazardType::bit_chasm);

    // The run is inert until started, then advances on fixed simulation steps.
    assert(first.state().status == RunStatus::ready);
    step_many(first, 120);
    assert(first.state().distance == 0.0F);
    first.step({.start = true}, kStep);
    assert(first.state().status == RunStatus::running);
    step_many(first, 60);
    assert(first.state().distance > 0.0F);
    assert(first.state().simulation_ticks == 61);

    // Lane requests clamp at the left/right boundaries.
    RunnerGame boundaries(0xFACEULL);
    boundaries.start();
    boundaries.step({.lane_delta = -1}, kStep);
    step_many(boundaries, 30);
    boundaries.step({.lane_delta = -1}, kStep);
    assert(boundaries.state().target_lane == 0);
    boundaries.step({.lane_delta = +1}, kStep);
    assert(boundaries.state().target_lane == 1);

    // The contextual action is unavailable early and does not behave like a sprint.
    RunnerGame early_action(0x12345678ULL);
    early_action.start();
    const float early_distance = early_action.state().distance;
    early_action.step({.action = true}, kStep);
    assert(!early_action.state().on_chasm_plate);
    assert(early_action.state().distance > early_distance);

    // BIT_CHASM: first press anchors a tile and pauses the runner on it.
    RunnerGame crossing(0x12345678ULL);
    std::uint64_t chasm_id = 0;
    reach_first_chasm(crossing, chasm_id);
    crossing.step({.action = true}, kStep);
    assert(crossing.state().status == RunStatus::running);
    assert(crossing.state().on_chasm_plate);
    assert(crossing.state().active_chasm_obstacle_id == chasm_id);
    assert(crossing.state().speed == 0.0F);
    const float plate_distance = crossing.state().distance;
    const std::uint64_t plate_score = crossing.state().score;

    step_many(crossing, 18);
    assert(crossing.state().on_chasm_plate);
    assert(crossing.state().distance == plate_distance);
    assert(crossing.state().score == plate_score);
    assert(crossing.state().chasm_plate_elapsed > 0.0F);

    // Second press blinks to solid ground beyond the gap and resumes the run.
    crossing.step({.action = true}, kStep);
    assert(!crossing.state().on_chasm_plate);
    assert(crossing.state().active_chasm_obstacle_id == 0);
    assert(crossing.state().status == RunStatus::running);
    assert(crossing.state().distance >
        plate_distance + crossing.config().chasm_exit_offset - 1.0F);
    assert(crossing.state().obstacles_cleared >= 2);
    assert(crossing.state().speed >= crossing.config().base_speed);

    // Missing the second press causes a fall, attributed to the BIT_CHASM itself.
    RunnerGame fall(0x12345678ULL);
    std::uint64_t fall_chasm_id = 0;
    reach_first_chasm(fall, fall_chasm_id);
    fall.step({.action = true}, kStep);
    assert(fall.state().on_chasm_plate);
    const float frozen_distance = fall.state().distance;
    step_many(fall, 120);
    assert(fall.state().status == RunStatus::crashed);
    assert(fall.state().collision_obstacle_id == fall_chasm_id);
    assert(fall.state().distance == frozen_distance);
    assert(!fall.state().on_chasm_plate);

    // The crash effect timer advances while distance, score, and tick count stay frozen.
    const auto frozen_score = fall.state().score;
    const auto frozen_ticks = fall.state().simulation_ticks;
    step_many(fall, 20);
    assert(fall.state().crash_effect_elapsed > 0.0F);
    assert(fall.state().distance == frozen_distance);
    assert(fall.state().score == frozen_score);
    assert(fall.state().simulation_ticks == frozen_ticks);
    assert(fall.state().collision_obstacle_id == fall_chasm_id);

    // A crashed run can be restarted with the same deterministic seed.
    fall.step({.restart = true}, kStep);
    assert(fall.state().status == RunStatus::running);
    assert(fall.state().distance == 0.0F);
    assert(fall.state().crash_effect_elapsed == 0.0F);
    assert(fall.obstacles().front().blocked_lane ==
        RunnerGame(0x12345678ULL).obstacles().front().blocked_lane);
    assert(fall.obstacles().front().type == HazardType::fold_gate);

    std::cout << "XNO Runner core smoke tests passed.\n";
    return 0;
}
