#include "sdl_runner_application.hpp"

#include "pd/core/game.hpp"
#include "pd/renderer/opengl_renderer.hpp"
#include "runner/core/runner_game.hpp"

#include <SDL3/SDL.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <string>

namespace xno::runner::platform {
namespace {
using Clock = std::chrono::steady_clock;
constexpr double kFixedStep = 1.0 / 60.0;
constexpr double kMaxFrameDelta = 0.25;

struct KeyboardState {
    bool sprint = false;
};

struct TouchState {
    SDL_FingerID stick_id = 0;
    SDL_FingerID sprint_id = 0;
    bool stick_active = false;
    bool sprint_active = false;
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

    const float sprint_dx = screen_x - aspect * 0.88F;
    const float sprint_dy = screen_y - 0.21F;
    if (!touch.sprint_active &&
        std::sqrt(sprint_dx * sprint_dx + sprint_dy * sprint_dy) <= 0.23F) {
        touch.sprint_active = true;
        touch.sprint_id = finger.fingerID;
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
    if (touch.sprint_active && touch.sprint_id == finger.fingerID) {
        touch.sprint_active = false;
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

    constexpr std::uint64_t kRunSeed = 0x584E4F52554E4E52ULL; // "XNORUNNR"
    RunnerGame game(kRunSeed);
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
    SDL_Log("Touch: drag the lower-left stick to change lanes; hold the lower-right ring to sprint.");
#else
    SDL_Log("Controls: SPACE start/restart, A/LEFT and D/RIGHT change lanes, SHIFT sprint, ESC quit.");
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
                    } else if (event.key.key == SDLK_LSHIFT || event.key.key == SDLK_RSHIFT) {
                        keyboard.sprint = true;
                    }
                    break;
                case SDL_EVENT_KEY_UP:
                    if (event.key.key == SDLK_LSHIFT || event.key.key == SDLK_RSHIFT) {
                        keyboard.sprint = false;
                    }
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
            input.sprint = keyboard.sprint || touch.sprint_active;
            game.step(input, static_cast<float>(kFixedStep));
            pending_lane_delta = 0;
            start_pending = false;
            restart_pending = false;
            accumulator -= kFixedStep;
        }

        int width = 0;
        int height = 0;
        SDL_GetWindowSizeInPixels(window, &width, &height);
        renderer.resize(width, height);

        pd::renderer::TouchHudState touch_hud{};
#if defined(__ANDROID__)
        touch_hud.visible = true;
#endif
        touch_hud.move_x = touch.move_x;
        touch_hud.move_y = touch.move_y;
        touch_hud.move_active = touch.stick_active;
        touch_hud.sprint_active = keyboard.sprint || touch.sprint_active;

        const auto render_state = make_render_state(game.state());
        renderer.render(render_state, static_cast<float>(game.state().elapsed_seconds), touch_hud);
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
            } else {
                const RunnerObstacle* next = nullptr;
                for (const auto& obstacle : game.obstacles()) {
                    if (!obstacle.resolved) { next = &obstacle; break; }
                }
                if (next != nullptr) {
                    std::snprintf(title, sizeof(title),
                        "XNO RUNNER // %.1f m // SCORE %llu // LANE %s // NEXT %s IN %.1f m",
                        state.distance, static_cast<unsigned long long>(state.score),
                        lane_name(state.target_lane), lane_name(next->blocked_lane),
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

    renderer.shutdown();
    SDL_GL_DestroyContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

} // namespace xno::runner::platform
