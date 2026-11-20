#pragma once

#include "runner/core/runner_game.hpp"

#include <memory>

namespace xno::runner::platform {

struct RunnerTouchOverlay {
    bool visible = false;
    float move_x = 0.0F;
    float move_y = 0.0F;
    bool move_active = false;
    bool sprint_active = false;
};

class RunnerHazardRenderer final {
public:
    RunnerHazardRenderer();
    ~RunnerHazardRenderer();

    RunnerHazardRenderer(const RunnerHazardRenderer&) = delete;
    RunnerHazardRenderer& operator=(const RunnerHazardRenderer&) = delete;

    [[nodiscard]] bool initialize();
    void resize(int width, int height) noexcept;
    void render(const RunnerGame& game, const RunnerTouchOverlay& touch);
    void shutdown() noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace xno::runner::platform
