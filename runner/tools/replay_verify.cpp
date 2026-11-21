#include "runner/core/runner_replay.hpp"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: xno_runner_replay_verify <path-to-xnr>\n";
        return 64;
    }

    const std::filesystem::path path =
        std::filesystem::u8path(argv[1]);
    std::ifstream input(path);
    if (!input) {
        std::cerr << "Could not open replay file: " << path.string() << '\n';
        return 66;
    }

    xno::runner::RunnerReplay replay{};
    std::string error;
    if (!xno::runner::read_runner_replay(input, replay, &error)) {
        std::cerr << "Invalid replay: " << error << '\n';
        return 65;
    }

    std::cout << "XNO RUNNER REPLAY\n"
              << "Format: " << replay.format_version << '\n'
              << "Ruleset: " << replay.ruleset_version << '\n'
              << "Seed: " << replay.seed << '\n'
              << "Frames: " << replay.frame_count << '\n'
              << "Duration: " << std::fixed << std::setprecision(2)
              << static_cast<double>(replay.frame_count) *
                    static_cast<double>(replay.fixed_delta_seconds)
              << " s\n"
              << "Input events: " << replay.events.size() << '\n'
              << "Checkpoints: " << replay.checkpoints.size() << '\n';

    const auto verification = xno::runner::verify_runner_replay(replay);
    if (!verification.success) {
        std::cerr << "REPLAY MISMATCH after frame "
                  << verification.mismatch_frame_count << ": "
                  << verification.error << '\n'
                  << "Expected hash: " << verification.expected_hash << '\n'
                  << "Actual hash:   " << verification.actual_hash << '\n';
        return 2;
    }

    std::cout << "VERIFIED: deterministic state matches all checkpoints.\n";
    return 0;
}
