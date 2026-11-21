#include "sdl_runner_application.hpp"

#include "pd/core/game.hpp"
#include "pd/renderer/opengl_renderer.hpp"
#include "runner/core/runner_game.hpp"
#include "runner/core/runner_replay.hpp"
#include "runner/renderer/runner_hazard_renderer.hpp"

#include <SDL3/SDL.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

namespace xno::runner::platform {
namespace {
using Clock = std::chrono::steady_clock;
constexpr double kFixedStep = 1.0 / 60.0;
constexpr double kMaxFrameDelta = 0.25;

struct KeyboardState {
    bool action_pending = false;
};

struct TouchState {
    SDL_FingerID stick_id = 0;
    SDL_FingerID action_id = 0;
    bool stick_active = false;
    bool action_active = false;
    bool action_pending = false;
    bool left_latched = false;
    bool right_latched = false;
    float move_x = 0.0F;
    float move_y = 0.0F;
};

void update_stick(TouchState& touch, float finger_x, float finger_y, float aspect) {
    constexpr float kCenterX = 0.16F;
    constexpr float kCenterY = 0.78F;
    constexpr float kRadius = 0.125F;
    const float dx = (finger_x - kCenterX) * aspect / kRadius;
    const float dy = (kCenterY - finger_y) / kRadius;
    const float magnitude = std::sqrt(dx * dx + dy * dy);
    constexpr float kDeadzone = 0.10F;
    if (magnitude <= kDeadzone) {
        touch.move_x = 0.0F;
        touch.move_y = 0.0F;
        return;
    }
    const float clamped = std::min(magnitude, 1.0F);
    const float remapped = (clamped - kDeadzone) / (1.0F - kDeadzone);
    const float scale = remapped / magnitude;
    touch.move_x = dx * scale;
    touch.move_y = dy * scale;
}

void finger_down(TouchState& touch, const SDL_TouchFingerEvent& finger, float aspect) {
    const float screen_x = finger.x * aspect;
    const float screen_y = 1.0F - finger.y;

    const float action_dx = screen_x - aspect * 0.88F;
    const float action_dy = screen_y - 0.21F;
    if (!touch.action_active &&
        std::sqrt(action_dx * action_dx + action_dy * action_dy) <= 0.23F) {
        touch.action_active = true;
        touch.action_pending = true;
        touch.action_id = finger.fingerID;
    }

    if (!touch.stick_active && screen_x < aspect * 0.43F && finger.y > 0.47F) {
        touch.stick_active = true;
        touch.stick_id = finger.fingerID;
        update_stick(touch, finger.x, finger.y, aspect);
    }
}

void finger_motion(TouchState& touch, const SDL_TouchFingerEvent& finger, float aspect) {
    if (touch.stick_active && touch.stick_id == finger.fingerID) {
        update_stick(touch, finger.x, finger.y, aspect);
    }
}

void finger_up(TouchState& touch, const SDL_TouchFingerEvent& finger) {
    if (touch.stick_active && touch.stick_id == finger.fingerID) {
        touch.stick_active = false;
        touch.move_x = 0.0F;
        touch.move_y = 0.0F;
        touch.left_latched = false;
        touch.right_latched = false;
    }
    if (touch.action_active && touch.action_id == finger.fingerID) {
        touch.action_active = false;
    }
}

void reset_touch(TouchState& touch) {
    touch = TouchState{};
}

const char* lane_name(int lane) noexcept {
    switch (lane) {
        case 0: return "LEFT";
        case 2: return "RIGHT";
        default: return "CENTER";
    }
}

const char* hazard_name(HazardType type) noexcept {
    switch (type) {
        case HazardType::bit_chasm: return "BIT CHASM";
        case HazardType::fold_gate: return "FOLD GATE";
    }
    return "UNKNOWN";
}

pd::core::GameState make_render_state(const RunnerState& state) {
    pd::core::GameState render_state{};
    render_state.player.x = 2.5F + state.lane_position * 2.65F;
    render_state.player.z = 2.5F + state.distance;
    render_state.player.yaw_radians = 0.0F;
    render_state.player.pitch_radians = 0.0F;
    render_state.player.speed = state.speed;
    render_state.elapsed_seconds = static_cast<double>(state.elapsed_seconds);
    render_state.simulation_ticks = state.simulation_ticks;
    return render_state;
}
} // namespace

int SdlRunnerApplication::run() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "SDL_Init failed: %s", SDL_GetError());
        return 1;
    }

#if defined(__ANDROID__)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight");
#else
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
#endif
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);

    SDL_Window* window = SDL_CreateWindow(
        "XNO RUNNER // SPACE TO START",
        1280,
        720,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (window == nullptr) {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "SDL_CreateWindow failed: %s", SDL_GetError());
        SDL_Quit();
        return 2;
    }

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    if (gl_context == nullptr) {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "SDL_GL_CreateContext failed: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 3;
    }
    if (!SDL_GL_MakeCurrent(window, gl_context)) {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "SDL_GL_MakeCurrent failed: %s", SDL_GetError());
        SDL_GL_DestroyContext(gl_context);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 4;
    }
    SDL_GL_SetSwapInterval(1);

    pd::renderer::OpenGLRenderer renderer;
    if (!renderer.initialize()) {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "Renderer initialization failed");
        SDL_GL_DestroyContext(gl_context);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 5;
    }

    RunnerHazardRenderer hazard_renderer;
    if (!hazard_renderer.initialize()) {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "Runner hazard renderer initialization failed");
        renderer.shutdown();
        SDL_GL_DestroyContext(gl_context);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 6;
    }

    constexpr std::uint64_t kRunSeed = 0x584E4F52554E4E52ULL; // "XNORUNNR"
    RunnerGame game(kRunSeed);
    RunnerReplayRecorder replay_recorder(
        kRunSeed, game.config(), static_cast<float>(kFixedStep));
    KeyboardState keyboard{};
    TouchState touch{};
    bool running = true;
    bool start_pending = false;
    bool restart_pending = false;
    int pending_lane_delta = 0;
    double accumulator = 0.0;
    auto previous_time = Clock::now();
    auto title_time = previous_time;

    SDL_Log("XNO Runner prototype initialized. Seed=%llu",
        static_cast<unsigned long long>(kRunSeed));
#if defined(__ANDROID__)
    SDL_Log("Touch: drag the lower-left stick to change lanes; tap the lower-right action ring for BIT_CHASM.");
#else
    SDL_Log("Controls: SPACE start/restart, A/LEFT and D/RIGHT change lanes, E contextual action, ESC quit.");
#endif

    while (running) {
        SDL_Event event{};
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_EVENT_QUIT:
                    running = false;
                    break;
                case SDL_EVENT_KEY_DOWN:
                    if (event.key.repeat) break;
                    if (event.key.key == SDLK_ESCAPE) {
                        running = false;
                    } else if (event.key.key == SDLK_SPACE) {
                        start_pending = game.state().status == RunStatus::ready;
                        restart_pending = game.state().status == RunStatus::crashed;
                    } else if (event.key.key == SDLK_A || event.key.key == SDLK_LEFT) {
                        pending_lane_delta = -1;
                    } else if (event.key.key == SDLK_D || event.key.key == SDLK_RIGHT) {
                        pending_lane_delta = +1;
                    } else if (event.key.key == SDLK_E) {
                        keyboard.action_pending = true;
                    }
                    break;
                case SDL_EVENT_KEY_UP:
                    break;
                case SDL_EVENT_FINGER_DOWN:
                case SDL_EVENT_FINGER_MOTION:
                case SDL_EVENT_FINGER_UP: {
                    int point_width = 1;
                    int point_height = 1;
                    SDL_GetWindowSize(window, &point_width, &point_height);
                    const float aspect = static_cast<float>(point_width) /
                        static_cast<float>(std::max(point_height, 1));
                    if (event.type == SDL_EVENT_FINGER_DOWN) {
                        if (game.state().status == RunStatus::ready) start_pending = true;
                        else if (game.state().status == RunStatus::crashed) restart_pending = true;
                        finger_down(touch, event.tfinger, aspect);
                    } else if (event.type == SDL_EVENT_FINGER_MOTION) {
                        finger_motion(touch, event.tfinger, aspect);
                    } else {
                        finger_up(touch, event.tfinger);
                    }
                    break;
                }
                case SDL_EVENT_WINDOW_FOCUS_LOST:
                    keyboard = KeyboardState{};
                    reset_touch(touch);
                    break;
                default:
                    break;
            }
        }

        if (touch.stick_active) {
            if (touch.move_x <= -0.55F && !touch.left_latched) {
                pending_lane_delta = -1;
                touch.left_latched = true;
            } else if (touch.move_x >= 0.55F && !touch.right_latched) {
                pending_lane_delta = +1;
                touch.right_latched = true;
            } else if (std::abs(touch.move_x) < 0.25F) {
                touch.left_latched = false;
                touch.right_latched = false;
            }
        }

        const auto now = Clock::now();
        const double frame_delta = std::chrono::duration<double>(now - previous_time).count();
        previous_time = now;
        accumulator += std::min(frame_delta, kMaxFrameDelta);

        while (accumulator >= kFixedStep) {
            RunnerInput input{};
            input.lane_delta = pending_lane_delta;
            input.start = start_pending;
            input.restart = restart_pending;
            input.action = keyboard.action_pending || touch.action_pending;
            replay_recorder.step(game, input);
            pending_lane_delta = 0;
            start_pending = false;
            restart_pending = false;
            keyboard.action_pending = false;
            touch.action_pending = false;
            accumulator -= kFixedStep;
        }

        int width = 0;
        int height = 0;
        SDL_GetWindowSizeInPixels(window, &width, &height);
        renderer.resize(width, height);
        hazard_renderer.resize(width, height);

        pd::renderer::TouchHudState touch_hud{};
#if defined(__ANDROID__)
        touch_hud.visible = true;
#endif
        touch_hud.move_x = touch.move_x;
        touch_hud.move_y = touch.move_y;
        touch_hud.move_active = touch.stick_active;

        RunnerTouchOverlay hazard_touch{};
        hazard_touch.visible = touch_hud.visible;
        hazard_touch.move_x = touch_hud.move_x;
        hazard_touch.move_y = touch_hud.move_y;
        hazard_touch.move_active = touch_hud.move_active;
        hazard_touch.action_available = game.action_available();
        hazard_touch.action_active = touch.action_active;
        hazard_touch.on_chasm_plate = game.state().on_chasm_plate;
        hazard_touch.plate_time_remaining = game.state().on_chasm_plate
            ? std::max(0.0F, game.config().chasm_plate_duration -
                game.state().chasm_plate_elapsed)
            : 0.0F;
        // Keep the upstream scene pass focused on the corridor; draw the touch
        // overlay last in the runner pass so it remains above nearby hazards.
        touch_hud.visible = false;

        const auto render_state = make_render_state(game.state());
        renderer.render(render_state, static_cast<float>(game.state().elapsed_seconds), touch_hud);
        hazard_renderer.render(game, hazard_touch);
        SDL_GL_SwapWindow(window);

        const double title_elapsed = std::chrono::duration<double>(now - title_time).count();
        if (title_elapsed >= 0.25) {
            const auto& state = game.state();
            char title[192]{};
            if (state.status == RunStatus::ready) {
                std::snprintf(title, sizeof(title), "XNO RUNNER // SPACE / TAP TO START // SEED %llu",
                    static_cast<unsigned long long>(game.seed()));
            } else if (state.status == RunStatus::crashed) {
                std::snprintf(title, sizeof(title),
                    "XNO RUNNER // CRASH // DIST %.1f m // SCORE %llu // SPACE TO RESTART",
                    state.distance, static_cast<unsigned long long>(state.score));
            } else if (state.on_chasm_plate) {
                std::snprintf(title, sizeof(title),
                    "XNO RUNNER // BLINK AGAIN // %.2f s",
                    std::max(0.0F, game.config().chasm_plate_duration -
                        state.chasm_plate_elapsed));
            } else {
                const RunnerObstacle* next = nullptr;
                for (const auto& obstacle : game.obstacles()) {
                    if (!obstacle.resolved) { next = &obstacle; break; }
                }
                if (next != nullptr) {
                    std::snprintf(title, sizeof(title),
                        "XNO RUNNER // %.1f m // SCORE %llu // LANE %s // NEXT %s / %s IN %.1f m",
                        state.distance, static_cast<unsigned long long>(state.score),
                        lane_name(state.target_lane), hazard_name(next->type),
                        lane_name(next->blocked_lane),
                        std::max(0.0F, next->distance - state.distance));
                } else {
                    std::snprintf(title, sizeof(title), "XNO RUNNER // %.1f m // SCORE %llu",
                        state.distance, static_cast<unsigned long long>(state.score));
                }
            }
            SDL_SetWindowTitle(window, title);
            title_time = now;
        }
    }

    replay_recorder.finish(game);
    if (replay_recorder.replay().frame_count > 0U) {
        char* preference_path = SDL_GetPrefPath("Skalagrimmo", "XNO-Runner");
        if (preference_path != nullptr) {
            const std::u8string preference_path_utf8(
                reinterpret_cast<const char8_t*>(preference_path));
            const std::filesystem::path replay_path =
                std::filesystem::path(preference_path_utf8) /
                std::filesystem::path(u8"last-run.xnr");
            std::ofstream replay_file(replay_path, std::ios::out | std::ios::trunc);
            std::string replay_error;
            if (replay_file &&
                write_runner_replay(replay_file, replay_recorder.replay(), &replay_error)) {
                const std::string replay_path_log =
                    std::string(preference_path) + "last-run.xnr";
                SDL_Log("Replay trace saved: %s", replay_path_log.c_str());
                SDL_Log("Replay trace: %llu frames, %llu input events, %llu checkpoints",
                    static_cast<unsigned long long>(replay_recorder.replay().frame_count),
                    static_cast<unsigned long long>(replay_recorder.replay().events.size()),
                    static_cast<unsigned long long>(replay_recorder.replay().checkpoints.size()));
            } else {
                SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not save replay trace: %s",
                    replay_error.empty() ? "file open failed" : replay_error.c_str());
            }
            SDL_free(preference_path);
        } else {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_GetPrefPath failed: %s", SDL_GetError());
        }
    }

    hazard_renderer.shutdown();
    renderer.shutdown();
    SDL_GL_DestroyContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

} // namespace xno::runner::platform
