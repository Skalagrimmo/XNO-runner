#include "runner/core/runner_game.hpp"
#include "runner/core/runner_replay.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <sstream>

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


    // Record a real scripted session: start, lane movement, both BIT_CHASM
    // presses, a FOLD_GATE crash, crash presentation, then a restart.
    const std::uint64_t replay_seed = 0x12345678ULL;
    RunnerGame recorded_game(replay_seed);
    xno::runner::RunnerReplayRecorder recorder(
        replay_seed, recorded_game.config(), kStep, 60);
    const auto record_step = [&recorder, &recorded_game](RunnerInput input) {
        recorder.step(recorded_game, input);
    };

    const auto recorded_first_fold = recorded_game.obstacles().front();
    const int replay_safe_delta = recorded_first_fold.blocked_lane == 1 ? -1 : 0;
    record_step({.start = true});
    if (replay_safe_delta != 0) {
        record_step({.lane_delta = replay_safe_delta});
    }

    for (int frame = 0;
         frame < 1200 && !recorded_game.action_available();
         ++frame) {
        record_step({});
        assert(recorded_game.state().status == RunStatus::running);
    }
    assert(recorded_game.action_available());
    record_step({.action = true});
    for (int frame = 0; frame < 18; ++frame) record_step({});
    record_step({.action = true});
    assert(!recorded_game.state().on_chasm_plate);

    xno::runner::RunnerObstacle next_fold{};
    bool found_next_fold = false;
    for (const auto& obstacle : recorded_game.obstacles()) {
        if (!obstacle.resolved) {
            next_fold = obstacle;
            found_next_fold = true;
            break;
        }
    }
    assert(found_next_fold);
    assert(next_fold.type == HazardType::fold_gate);

    for (int frame = 0;
         frame < 1200 &&
         next_fold.distance - recorded_game.state().distance > 10.0F;
         ++frame) {
        record_step({});
    }

    while (recorded_game.state().target_lane != next_fold.blocked_lane) {
        const int delta = next_fold.blocked_lane > recorded_game.state().target_lane
            ? 1 : -1;
        record_step({.lane_delta = delta});
    }
    for (int frame = 0;
         frame < 1200 && recorded_game.state().status != RunStatus::crashed;
         ++frame) {
        record_step({});
    }
    assert(recorded_game.state().status == RunStatus::crashed);
    for (int frame = 0; frame < 20; ++frame) record_step({});
    record_step({.restart = true});
    assert(recorded_game.state().status == RunStatus::running);
    for (int frame = 0; frame < 30; ++frame) record_step({});
    recorder.finish(recorded_game);

    const auto& captured_replay = recorder.replay();
    assert(captured_replay.frame_count > 60);
    assert(captured_replay.events.size() >= 5);
    assert(!captured_replay.checkpoints.empty());
    assert(captured_replay.checkpoints.back().frame_count == captured_replay.frame_count);

    std::stringstream replay_stream;
    std::string replay_error;
    assert(xno::runner::write_runner_replay(
        replay_stream, captured_replay, &replay_error));
    replay_stream.seekg(0);
    xno::runner::RunnerReplay decoded_replay{};
    assert(xno::runner::read_runner_replay(
        replay_stream, decoded_replay, &replay_error));
    assert(decoded_replay.frame_count == captured_replay.frame_count);
    assert(decoded_replay.events.size() == captured_replay.events.size());
    assert(decoded_replay.checkpoints.size() == captured_replay.checkpoints.size());

    const auto replay_verification = xno::runner::verify_runner_replay(decoded_replay);
    assert(replay_verification.success);
    assert(replay_verification.frames_processed == captured_replay.frame_count);

    // Altering an input must be detected at the first affected state checkpoint.
    auto tampered_replay = decoded_replay;
    const auto action_event = std::find_if(
        tampered_replay.events.begin(),
        tampered_replay.events.end(),
        [](const xno::runner::RunnerReplayEvent& event) {
            return event.input.action;
        });
    assert(action_event != tampered_replay.events.end());
    action_event->input.action = false;
    const auto tampered_verification = xno::runner::verify_runner_replay(tampered_replay);
    assert(!tampered_verification.success);
    assert(tampered_verification.mismatch_frame_count > 0);

    std::stringstream malformed_replay("XNO_RUNNER_REPLAY 99\n");
    xno::runner::RunnerReplay rejected_replay{};
    assert(!xno::runner::read_runner_replay(
        malformed_replay, rejected_replay, &replay_error));

    std::cout << "XNO Runner core and replay smoke tests passed.\n";
    return 0;
}
