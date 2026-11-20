#include "runner/renderer/runner_hazard_renderer.hpp"

#include <SDL3/SDL.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

namespace xno::runner::platform {
namespace {

#if defined(_WIN32)
#define XNO_GL_CALL __stdcall
#else
#define XNO_GL_CALL
#endif

using GLenum = unsigned int;
using GLuint = unsigned int;
using GLint = int;
using GLsizei = int;
using GLboolean = unsigned char;
using GLsizeiptr = std::ptrdiff_t;

constexpr GLenum kVertexShader = 0x8B31U;
constexpr GLenum kFragmentShader = 0x8B30U;
constexpr GLenum kCompileStatus = 0x8B81U;
constexpr GLenum kLinkStatus = 0x8B82U;
constexpr GLenum kInfoLogLength = 0x8B84U;
constexpr GLenum kArrayBuffer = 0x8892U;
constexpr GLenum kDynamicDraw = 0x88E8U;
constexpr GLenum kFloat = 0x1406U;
constexpr GLenum kLines = 0x0001U;
constexpr GLenum kTriangles = 0x0004U;
constexpr GLenum kDepthTest = 0x0B71U;
constexpr GLenum kLequal = 0x0203U;
constexpr float kPi = 3.14159265358979323846F;
constexpr float kNearPlane = 0.10F;
constexpr float kFarPlane = 90.0F;
constexpr float kHazardWakeDistance = 14.0F;
constexpr float kHazardDeployDistance = 4.0F;

struct Vertex {
    float x;
    float y;
    float z;
    float r;
    float g;
    float b;
};

struct Vec3 {
    float x;
    float y;
    float z;
};

struct CameraPoint {
    float x;
    float y;
    float z;
};

struct Color {
    float r;
    float g;
    float b;
};

Color mix_color(const Color& a, const Color& b, float amount) noexcept {
    const float t = std::clamp(amount, 0.0F, 1.0F);
    return {
        a.r + (b.r - a.r) * t,
        a.g + (b.g - a.g) * t,
        a.b + (b.b - a.b) * t
    };
}

float smooth_step(float edge0, float edge1, float value) noexcept {
    const float t = std::clamp((value - edge0) / (edge1 - edge0), 0.0F, 1.0F);
    return t * t * (3.0F - 2.0F * t);
}

std::uint64_t mix64(std::uint64_t value) noexcept {
    value ^= value >> 30U;
    value *= 0xBF58476D1CE4E5B9ULL;
    value ^= value >> 27U;
    value *= 0x94D049BB133111EBULL;
    return value ^ (value >> 31U);
}

float unit_hash(std::uint64_t value) noexcept {
    return static_cast<float>(mix64(value) & 0xFFFFFFULL) /
        static_cast<float>(0x1000000ULL);
}

struct GlApi {
    using CreateShaderFn = GLuint (XNO_GL_CALL *)(GLenum);
    using ShaderSourceFn = void (XNO_GL_CALL *)(GLuint, GLsizei, const char* const*, const GLint*);
    using CompileShaderFn = void (XNO_GL_CALL *)(GLuint);
    using GetShaderivFn = void (XNO_GL_CALL *)(GLuint, GLenum, GLint*);
    using GetShaderInfoLogFn = void (XNO_GL_CALL *)(GLuint, GLsizei, GLsizei*, char*);
    using DeleteShaderFn = void (XNO_GL_CALL *)(GLuint);
    using CreateProgramFn = GLuint (XNO_GL_CALL *)();
    using AttachShaderFn = void (XNO_GL_CALL *)(GLuint, GLuint);
    using BindAttribLocationFn = void (XNO_GL_CALL *)(GLuint, GLuint, const char*);
    using LinkProgramFn = void (XNO_GL_CALL *)(GLuint);
    using GetProgramivFn = void (XNO_GL_CALL *)(GLuint, GLenum, GLint*);
    using GetProgramInfoLogFn = void (XNO_GL_CALL *)(GLuint, GLsizei, GLsizei*, char*);
    using DeleteProgramFn = void (XNO_GL_CALL *)(GLuint);
    using GenBuffersFn = void (XNO_GL_CALL *)(GLsizei, GLuint*);
    using BindBufferFn = void (XNO_GL_CALL *)(GLenum, GLuint);
    using BufferDataFn = void (XNO_GL_CALL *)(GLenum, GLsizeiptr, const void*, GLenum);
    using DeleteBuffersFn = void (XNO_GL_CALL *)(GLsizei, const GLuint*);
    using UseProgramFn = void (XNO_GL_CALL *)(GLuint);
    using EnableVertexAttribArrayFn = void (XNO_GL_CALL *)(GLuint);
    using DisableVertexAttribArrayFn = void (XNO_GL_CALL *)(GLuint);
    using VertexAttribPointerFn = void (XNO_GL_CALL *)(GLuint, GLint, GLenum, GLboolean, GLsizei, const void*);
    using DrawArraysFn = void (XNO_GL_CALL *)(GLenum, GLint, GLsizei);
    using EnableFn = void (XNO_GL_CALL *)(GLenum);
    using DisableFn = void (XNO_GL_CALL *)(GLenum);
    using DepthFuncFn = void (XNO_GL_CALL *)(GLenum);

    CreateShaderFn create_shader = nullptr;
    ShaderSourceFn shader_source = nullptr;
    CompileShaderFn compile_shader = nullptr;
    GetShaderivFn get_shader_iv = nullptr;
    GetShaderInfoLogFn get_shader_info_log = nullptr;
    DeleteShaderFn delete_shader = nullptr;
    CreateProgramFn create_program = nullptr;
    AttachShaderFn attach_shader = nullptr;
    BindAttribLocationFn bind_attrib_location = nullptr;
    LinkProgramFn link_program = nullptr;
    GetProgramivFn get_program_iv = nullptr;
    GetProgramInfoLogFn get_program_info_log = nullptr;
    DeleteProgramFn delete_program = nullptr;
    GenBuffersFn gen_buffers = nullptr;
    BindBufferFn bind_buffer = nullptr;
    BufferDataFn buffer_data = nullptr;
    DeleteBuffersFn delete_buffers = nullptr;
    UseProgramFn use_program = nullptr;
    EnableVertexAttribArrayFn enable_vertex_attrib_array = nullptr;
    DisableVertexAttribArrayFn disable_vertex_attrib_array = nullptr;
    VertexAttribPointerFn vertex_attrib_pointer = nullptr;
    DrawArraysFn draw_arrays = nullptr;
    EnableFn enable = nullptr;
    DisableFn disable = nullptr;
    DepthFuncFn depth_func = nullptr;

    template <typename Function>
    bool load_one(Function& function, const char* name) noexcept {
        function = reinterpret_cast<Function>(SDL_GL_GetProcAddress(name));
        if (function == nullptr) {
            SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Missing OpenGL entry point: %s", name);
            return false;
        }
        return true;
    }

    bool load() noexcept {
#define XNO_LOAD_GL(member, symbol) if (!load_one(member, symbol)) return false
        XNO_LOAD_GL(create_shader, "glCreateShader");
        XNO_LOAD_GL(shader_source, "glShaderSource");
        XNO_LOAD_GL(compile_shader, "glCompileShader");
        XNO_LOAD_GL(get_shader_iv, "glGetShaderiv");
        XNO_LOAD_GL(get_shader_info_log, "glGetShaderInfoLog");
        XNO_LOAD_GL(delete_shader, "glDeleteShader");
        XNO_LOAD_GL(create_program, "glCreateProgram");
        XNO_LOAD_GL(attach_shader, "glAttachShader");
        XNO_LOAD_GL(bind_attrib_location, "glBindAttribLocation");
        XNO_LOAD_GL(link_program, "glLinkProgram");
        XNO_LOAD_GL(get_program_iv, "glGetProgramiv");
        XNO_LOAD_GL(get_program_info_log, "glGetProgramInfoLog");
        XNO_LOAD_GL(delete_program, "glDeleteProgram");
        XNO_LOAD_GL(gen_buffers, "glGenBuffers");
        XNO_LOAD_GL(bind_buffer, "glBindBuffer");
        XNO_LOAD_GL(buffer_data, "glBufferData");
        XNO_LOAD_GL(delete_buffers, "glDeleteBuffers");
        XNO_LOAD_GL(use_program, "glUseProgram");
        XNO_LOAD_GL(enable_vertex_attrib_array, "glEnableVertexAttribArray");
        XNO_LOAD_GL(disable_vertex_attrib_array, "glDisableVertexAttribArray");
        XNO_LOAD_GL(vertex_attrib_pointer, "glVertexAttribPointer");
        XNO_LOAD_GL(draw_arrays, "glDrawArrays");
        XNO_LOAD_GL(enable, "glEnable");
        XNO_LOAD_GL(disable, "glDisable");
        XNO_LOAD_GL(depth_func, "glDepthFunc");
#undef XNO_LOAD_GL
        return true;
    }
};

constexpr const char* kVertexShaderEs = R"GLSL(
attribute vec3 aPosition;
attribute vec3 aColor;
varying vec3 vColor;
void main() {
    gl_Position = vec4(aPosition, 1.0);
    vColor = aColor;
}
)GLSL";

constexpr const char* kFragmentShaderEs = R"GLSL(
precision mediump float;
varying vec3 vColor;
void main() {
    gl_FragColor = vec4(vColor, 1.0);
}
)GLSL";

constexpr const char* kVertexShaderDesktop = R"GLSL(
#version 120
attribute vec3 aPosition;
attribute vec3 aColor;
varying vec3 vColor;
void main() {
    gl_Position = vec4(aPosition, 1.0);
    vColor = aColor;
}
)GLSL";

constexpr const char* kFragmentShaderDesktop = R"GLSL(
#version 120
varying vec3 vColor;
void main() {
    gl_FragColor = vec4(vColor, 1.0);
}
)GLSL";

GLuint compile_shader(GlApi& api, GLenum type, const char* source) {
    const GLuint shader = api.create_shader(type);
    if (shader == 0U) return 0U;

    api.shader_source(shader, 1, &source, nullptr);
    api.compile_shader(shader);

    GLint compiled = 0;
    api.get_shader_iv(shader, kCompileStatus, &compiled);
    if (compiled == 0) {
        GLint log_length = 0;
        api.get_shader_iv(shader, kInfoLogLength, &log_length);
        std::string log(static_cast<std::size_t>(std::max(log_length, 1)), '\0');
        api.get_shader_info_log(shader, static_cast<GLsizei>(log.size()), nullptr, log.data());
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Runner hazard shader failed: %s", log.c_str());
        api.delete_shader(shader);
        return 0U;
    }
    return shader;
}

GLuint create_program(GlApi& api, GLuint vertex_shader, GLuint fragment_shader) {
    const GLuint program = api.create_program();
    if (program == 0U) return 0U;

    api.attach_shader(program, vertex_shader);
    api.attach_shader(program, fragment_shader);
    api.bind_attrib_location(program, 0U, "aPosition");
    api.bind_attrib_location(program, 1U, "aColor");
    api.link_program(program);

    GLint linked = 0;
    api.get_program_iv(program, kLinkStatus, &linked);
    if (linked == 0) {
        GLint log_length = 0;
        api.get_program_iv(program, kInfoLogLength, &log_length);
        std::string log(static_cast<std::size_t>(std::max(log_length, 1)), '\0');
        api.get_program_info_log(program, static_cast<GLsizei>(log.size()), nullptr, log.data());
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Runner hazard program failed: %s", log.c_str());
        api.delete_program(program);
        return 0U;
    }
    return program;
}

class SceneBuilder final {
public:
    SceneBuilder(
        const RunnerGame& game,
        int width,
        int height,
        std::vector<Vertex>& fill_vertices,
        std::vector<Vertex>& line_vertices)
        : game_(game),
          camera_x_(2.5F + game.state().lane_position * 2.65F),
          camera_y_(1.62F),
          camera_z_(2.5F + game.state().distance),
          width_(static_cast<float>(std::max(width, 1))),
          height_(static_cast<float>(std::max(height, 1))),
          focal_pixels_(height_ * 0.82F),
          fill_vertices_(fill_vertices),
          line_vertices_(line_vertices) {}

    void build(const RunnerTouchOverlay& touch) {
        if (game_.state().status == RunStatus::crashed) {
            add_crash_effect();
        } else {
            for (const RunnerObstacle& obstacle : game_.obstacles()) {
                if (obstacle.resolved) continue;
                const float approach = obstacle.distance - game_.state().distance;
                if (approach < -1.5F || approach > 28.0F) continue;
                const float progress = std::clamp(
                    (kHazardWakeDistance - approach) /
                        (kHazardWakeDistance - kHazardDeployDistance),
                    0.0F,
                    1.0F);
                if (obstacle.type == HazardType::fold_gate) {
                    add_fold_gate(obstacle, approach, progress);
                } else {
                    add_bit_chasm(obstacle, approach, progress);
                }
            }
        }
        add_game_hud();
        if (touch.visible) add_touch_hud(touch);
    }

private:
    [[nodiscard]] CameraPoint to_camera(const Vec3& point) const noexcept {
        return {
            point.x - camera_x_,
            point.y - camera_y_,
            point.z - camera_z_
        };
    }

    [[nodiscard]] bool project(
        const CameraPoint& point,
        Vertex& out,
        const Color& color) const noexcept {
        if (point.z < kNearPlane || point.z > kFarPlane) return false;
        const float inverse_z = 1.0F / point.z;
        const float focal_x = focal_pixels_ * 2.0F / width_;
        const float focal_y = focal_pixels_ * 2.0F / height_;
        const float ndc_x = point.x * inverse_z * focal_x;
        const float ndc_y = point.y * inverse_z * focal_y;
        if (std::abs(ndc_x) > 5.0F || std::abs(ndc_y) > 5.0F) return false;

        const float near_far = (kFarPlane + kNearPlane) / (kFarPlane - kNearPlane);
        const float depth_term = (2.0F * kFarPlane * kNearPlane) /
            ((kFarPlane - kNearPlane) * point.z);
        out = {
            ndc_x,
            ndc_y,
            near_far - depth_term,
            color.r,
            color.g,
            color.b
        };
        return true;
    }

    void add_line(const Vec3& world_a, const Vec3& world_b, const Color& color) {
        CameraPoint a = to_camera(world_a);
        CameraPoint b = to_camera(world_b);
        if (a.z < kNearPlane && b.z < kNearPlane) return;
        if (a.z > kFarPlane && b.z > kFarPlane) return;

        if (a.z < kNearPlane) {
            const float t = (kNearPlane - a.z) / (b.z - a.z);
            a.x += (b.x - a.x) * t;
            a.y += (b.y - a.y) * t;
            a.z = kNearPlane;
        }
        if (b.z < kNearPlane) {
            const float t = (kNearPlane - b.z) / (a.z - b.z);
            b.x += (a.x - b.x) * t;
            b.y += (a.y - b.y) * t;
            b.z = kNearPlane;
        }
        if (a.z > kFarPlane) {
            const float t = (kFarPlane - a.z) / (b.z - a.z);
            a.x += (b.x - a.x) * t;
            a.y += (b.y - a.y) * t;
            a.z = kFarPlane;
        }
        if (b.z > kFarPlane) {
            const float t = (kFarPlane - b.z) / (a.z - b.z);
            b.x += (a.x - b.x) * t;
            b.y += (a.y - b.y) * t;
            b.z = kFarPlane;
        }

        Vertex vertex_a{};
        Vertex vertex_b{};
        if (project(a, vertex_a, color) && project(b, vertex_b, color)) {
            line_vertices_.push_back(vertex_a);
            line_vertices_.push_back(vertex_b);
        }
    }

    void add_triangle(const Vec3& a, const Vec3& b, const Vec3& c, const Color& color) {
        const CameraPoint camera_a = to_camera(a);
        const CameraPoint camera_b = to_camera(b);
        const CameraPoint camera_c = to_camera(c);
        Vertex vertex_a{};
        Vertex vertex_b{};
        Vertex vertex_c{};
        if (project(camera_a, vertex_a, color) &&
            project(camera_b, vertex_b, color) &&
            project(camera_c, vertex_c, color)) {
            fill_vertices_.push_back(vertex_a);
            fill_vertices_.push_back(vertex_b);
            fill_vertices_.push_back(vertex_c);
        }
    }

    void add_quad(
        float min_x,
        float min_y,
        float min_z,
        float max_x,
        float max_y,
        float max_z,
        const Color& color) {
        const Vec3 a{min_x, min_y, min_z};
        const Vec3 b{max_x, min_y, min_z};
        const Vec3 c{max_x, max_y, max_z};
        const Vec3 d{min_x, max_y, max_z};
        add_triangle(a, b, c, color);
        add_triangle(a, c, d, color);
    }

    void add_screen_line(
        float x0,
        float y0,
        float x1,
        float y1,
        const Color& color) {
        line_vertices_.push_back({
            x0 * 2.0F - 1.0F, y0 * 2.0F - 1.0F, -0.985F,
            color.r, color.g, color.b
        });
        line_vertices_.push_back({
            x1 * 2.0F - 1.0F, y1 * 2.0F - 1.0F, -0.985F,
            color.r, color.g, color.b
        });
    }

    [[nodiscard]] static std::array<std::uint8_t, 7> glyph_rows(char character) noexcept {
        switch (character) {
            case '0': return {0x0EU, 0x11U, 0x13U, 0x15U, 0x19U, 0x11U, 0x0EU};
            case '1': return {0x04U, 0x0CU, 0x04U, 0x04U, 0x04U, 0x04U, 0x0EU};
            case '2': return {0x0EU, 0x11U, 0x01U, 0x02U, 0x04U, 0x08U, 0x1FU};
            case '3': return {0x1EU, 0x01U, 0x01U, 0x0EU, 0x01U, 0x01U, 0x1EU};
            case '4': return {0x02U, 0x06U, 0x0AU, 0x12U, 0x1FU, 0x02U, 0x02U};
            case '5': return {0x1FU, 0x10U, 0x10U, 0x1EU, 0x01U, 0x01U, 0x1EU};
            case '6': return {0x06U, 0x08U, 0x10U, 0x1EU, 0x11U, 0x11U, 0x0EU};
            case '7': return {0x1FU, 0x01U, 0x02U, 0x04U, 0x08U, 0x08U, 0x08U};
            case '8': return {0x0EU, 0x11U, 0x11U, 0x0EU, 0x11U, 0x11U, 0x0EU};
            case '9': return {0x0EU, 0x11U, 0x11U, 0x0FU, 0x01U, 0x02U, 0x0CU};
            case 'A': return {0x0EU, 0x11U, 0x11U, 0x1FU, 0x11U, 0x11U, 0x11U};
            case 'B': return {0x1EU, 0x11U, 0x11U, 0x1EU, 0x11U, 0x11U, 0x1EU};
            case 'C': return {0x0EU, 0x11U, 0x10U, 0x10U, 0x10U, 0x11U, 0x0EU};
            case 'D': return {0x1CU, 0x12U, 0x11U, 0x11U, 0x11U, 0x12U, 0x1CU};
            case 'E': return {0x1FU, 0x10U, 0x10U, 0x1EU, 0x10U, 0x10U, 0x1FU};
            case 'F': return {0x1FU, 0x10U, 0x10U, 0x1EU, 0x10U, 0x10U, 0x10U};
            case 'G': return {0x0EU, 0x11U, 0x10U, 0x17U, 0x11U, 0x11U, 0x0FU};
            case 'H': return {0x11U, 0x11U, 0x11U, 0x1FU, 0x11U, 0x11U, 0x11U};
            case 'I': return {0x0EU, 0x04U, 0x04U, 0x04U, 0x04U, 0x04U, 0x0EU};
            case 'J': return {0x07U, 0x02U, 0x02U, 0x02U, 0x12U, 0x12U, 0x0CU};
            case 'K': return {0x11U, 0x12U, 0x14U, 0x18U, 0x14U, 0x12U, 0x11U};
            case 'L': return {0x10U, 0x10U, 0x10U, 0x10U, 0x10U, 0x10U, 0x1FU};
            case 'M': return {0x11U, 0x1BU, 0x15U, 0x15U, 0x11U, 0x11U, 0x11U};
            case 'N': return {0x11U, 0x19U, 0x15U, 0x13U, 0x11U, 0x11U, 0x11U};
            case 'O': return {0x0EU, 0x11U, 0x11U, 0x11U, 0x11U, 0x11U, 0x0EU};
            case 'P': return {0x1EU, 0x11U, 0x11U, 0x1EU, 0x10U, 0x10U, 0x10U};
            case 'Q': return {0x0EU, 0x11U, 0x11U, 0x11U, 0x15U, 0x12U, 0x0DU};
            case 'R': return {0x1EU, 0x11U, 0x11U, 0x1EU, 0x14U, 0x12U, 0x11U};
            case 'S': return {0x0FU, 0x10U, 0x10U, 0x0EU, 0x01U, 0x01U, 0x1EU};
            case 'T': return {0x1FU, 0x04U, 0x04U, 0x04U, 0x04U, 0x04U, 0x04U};
            case 'U': return {0x11U, 0x11U, 0x11U, 0x11U, 0x11U, 0x11U, 0x0EU};
            case 'V': return {0x11U, 0x11U, 0x11U, 0x11U, 0x11U, 0x0AU, 0x04U};
            case 'W': return {0x11U, 0x11U, 0x11U, 0x15U, 0x15U, 0x15U, 0x0AU};
            case 'X': return {0x11U, 0x11U, 0x0AU, 0x04U, 0x0AU, 0x11U, 0x11U};
            case 'Y': return {0x11U, 0x11U, 0x0AU, 0x04U, 0x04U, 0x04U, 0x04U};
            case 'Z': return {0x1FU, 0x01U, 0x02U, 0x04U, 0x08U, 0x10U, 0x1FU};
            case ':': return {0x00U, 0x04U, 0x04U, 0x00U, 0x04U, 0x04U, 0x00U};
            case '.': return {0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x0CU, 0x0CU};
            case '-': return {0x00U, 0x00U, 0x00U, 0x1FU, 0x00U, 0x00U, 0x00U};
            case '/': return {0x01U, 0x02U, 0x02U, 0x04U, 0x08U, 0x08U, 0x10U};
            default: return {};
        }
    }

    void add_screen_text(
        std::string_view text,
        float x,
        float top_y,
        float cell_height,
        const Color& color) {
        const float cell_width = cell_height * (height_ / width_);
        const float character_advance = cell_width * 6.0F;
        for (std::size_t index = 0; index < text.size(); ++index) {
            const auto rows = glyph_rows(text[index]);
            const float character_x = x + static_cast<float>(index) * character_advance;
            for (std::size_t row = 0; row < rows.size(); ++row) {
                for (int column = 0; column < 5; ++column) {
                    const std::uint8_t mask = static_cast<std::uint8_t>(1U << (4 - column));
                    if ((rows[row] & mask) == 0U) continue;
                    const float pixel_x = character_x +
                        static_cast<float>(column) * cell_width;
                    const float pixel_y = top_y -
                        (static_cast<float>(row) + 0.5F) * cell_height;
                    const float pixel_width = cell_width * 0.78F;
                    const float thickness = cell_height * 0.12F;
                    add_screen_line(pixel_x, pixel_y - thickness,
                        pixel_x + pixel_width, pixel_y - thickness, color);
                    add_screen_line(pixel_x, pixel_y + thickness,
                        pixel_x + pixel_width, pixel_y + thickness, color);
                }
            }
        }
    }

    void add_centered_screen_text(
        std::string_view text,
        float center_x,
        float top_y,
        float cell_height,
        const Color& color) {
        const float cell_width = cell_height * (height_ / width_);
        const float total_width = static_cast<float>(text.size()) * cell_width * 6.0F;
        add_screen_text(text, center_x - total_width * 0.5F,
            top_y, cell_height, color);
    }

    void add_game_hud() {
        constexpr Color cyan{0.08F, 0.72F, 0.82F};
        constexpr Color muted{0.035F, 0.28F, 0.36F};
        constexpr Color amber{1.0F, 0.55F, 0.12F};
        constexpr Color danger{1.0F, 0.18F, 0.30F};
        const RunnerState& state = game_.state();

        if (state.status == RunStatus::ready) {
            add_centered_screen_text("XNO RUNNER", 0.5F, 0.60F, 0.014F, cyan);
            add_centered_screen_text("TAP TO START", 0.5F, 0.50F, 0.008F, amber);
            return;
        }

        char distance_text[24]{};
        char score_text[24]{};
        std::snprintf(distance_text, sizeof(distance_text),
            "DIST %.1fM", static_cast<double>(state.distance));
        std::snprintf(score_text, sizeof(score_text),
            "SCORE %07llu", static_cast<unsigned long long>(state.score));

        add_screen_text(distance_text, 0.035F, 0.965F, 0.0055F, cyan);
        add_screen_text(score_text, 0.035F, 0.905F, 0.0055F, amber);

        const char* lane = state.target_lane == 0
            ? "LEFT" : (state.target_lane == 2 ? "RIGHT" : "CENTER");
        char lane_text[20]{};
        std::snprintf(lane_text, sizeof(lane_text), "LANE %s", lane);
        add_screen_text(lane_text, 0.665F, 0.965F, 0.0055F, cyan);

        const RunnerObstacle* next = nullptr;
        for (const RunnerObstacle& obstacle : game_.obstacles()) {
            if (!obstacle.resolved) {
                next = &obstacle;
                break;
            }
        }

        char next_text[32]{};
        if (next != nullptr) {
            const double approach = std::max(
                0.0F, next->distance - state.distance);
            if (next->type == HazardType::bit_chasm) {
                std::snprintf(next_text, sizeof(next_text),
                    "NEXT BIT CHASM %.1fM", approach);
            } else {
                const char* side = next->blocked_lane == 0
                    ? "L" : (next->blocked_lane == 2 ? "R" : "C");
                std::snprintf(next_text, sizeof(next_text),
                    "NEXT FOLD %s %.1fM", side, approach);
            }
        } else {
            std::snprintf(next_text, sizeof(next_text), "NEXT CLEAR");
        }
        add_screen_text(next_text, 0.665F, 0.905F, 0.0050F, muted);

        if (state.status == RunStatus::crashed) {
            add_centered_screen_text("RUN TERMINATED", 0.5F, 0.69F,
                0.0085F, danger);
            add_centered_screen_text("TAP TO RESTART", 0.5F, 0.60F,
                0.0065F, amber);
            return;
        }

        if (state.on_chasm_plate) {
            char blink_text[24]{};
            const float remaining = std::max(
                0.0F, game_.config().chasm_plate_duration - state.chasm_plate_elapsed);
            std::snprintf(blink_text, sizeof(blink_text), "BLINK %.2FS",
                static_cast<double>(remaining));
            const float urgency = 1.0F - std::clamp(
                remaining / game_.config().chasm_plate_duration, 0.0F, 1.0F);
            add_centered_screen_text("BIT CHASM", 0.5F, 0.80F, 0.0070F, amber);
            add_centered_screen_text(blink_text, 0.5F, 0.73F, 0.0100F,
                mix_color(amber, danger, urgency));
        } else if (game_.action_available()) {
            add_centered_screen_text("ANCHOR", 0.5F, 0.80F, 0.0100F, amber);
        }
    }

    void add_screen_square(float cx, float cy, float half_size, const Color& color) {
        const float left = cx - half_size;
        const float right = cx + half_size;
        const float bottom = cy - half_size;
        const float top = cy + half_size;
        add_screen_line(left, bottom, right, bottom, color);
        add_screen_line(right, bottom, right, top, color);
        add_screen_line(right, top, left, top, color);
        add_screen_line(left, top, left, bottom, color);
    }

    void add_screen_circle(float cx, float cy, float radius, const Color& color) {
        add_screen_arc(cx, cy, radius, 0.0F, 2.0F * kPi, color, 28);
    }

    void add_screen_arc(
        float cx,
        float cy,
        float radius,
        float start_angle,
        float end_angle,
        const Color& color,
        int segments) {
        const float inverse_aspect = height_ / width_;
        const int count = std::max(segments, 1);
        for (int index = 0; index < count; ++index) {
            const float t0 = static_cast<float>(index) / static_cast<float>(count);
            const float t1 = static_cast<float>(index + 1) / static_cast<float>(count);
            const float angle0 = start_angle + (end_angle - start_angle) * t0;
            const float angle1 = start_angle + (end_angle - start_angle) * t1;
            add_screen_line(
                cx + std::cos(angle0) * radius * inverse_aspect,
                cy + std::sin(angle0) * radius,
                cx + std::cos(angle1) * radius * inverse_aspect,
                cy + std::sin(angle1) * radius,
                color);
        }
    }

    void add_hinge(float x, float y, float z, const Color& color) {
        constexpr float radius = 0.13F;
        add_line({x - radius, y, z}, {x, y + radius, z}, color);
        add_line({x, y + radius, z}, {x + radius, y, z}, color);
        add_line({x + radius, y, z}, {x, y - radius, z}, color);
        add_line({x, y - radius, z}, {x - radius, y, z}, color);
        add_line({x - radius * 0.55F, y, z - 0.02F},
                 {x + radius * 0.55F, y, z - 0.02F}, color);
    }

    void add_fold_gate(
        const RunnerObstacle& obstacle,
        float approach,
        float progress) {
        constexpr Color cyan{0.04F, 0.58F, 0.68F};
        constexpr Color amber{1.0F, 0.47F, 0.10F};
        constexpr Color danger{1.0F, 0.16F, 0.34F};
        const float center_x = 2.5F +
            static_cast<float>(obstacle.blocked_lane - 1) * 2.65F;
        const float z = 2.5F + obstacle.distance;
        const float deploy = smooth_step(0.0F, 1.0F, progress);
        const Color mechanism = mix_color(cyan, amber, progress);
        const Color active = mix_color(amber, danger, smooth_step(0.72F, 1.0F, deploy));

        const float left = center_x - 1.15F;
        const float right = center_x + 1.15F;
        add_line({left, 0.05F, z}, {left, 4.05F, z}, mechanism);
        add_line({right, 0.05F, z}, {right, 4.05F, z}, mechanism);
        add_line({left, 4.05F, z}, {right, 4.05F, z}, mechanism);
        add_line({left, 0.05F, z}, {right, 0.05F, z}, mechanism);
        add_line({left, 2.15F, z}, {left + 0.22F, 2.15F, z - 0.10F}, cyan);
        add_line({right, 2.15F, z}, {right - 0.22F, 2.15F, z - 0.10F}, cyan);

        constexpr float hinge_y = 1.62F;
        constexpr float arm_length = 2.08F;
        const float angle = (1.0F - deploy) * 1.15F;
        const float arm_dx = std::cos(angle) * arm_length;
        const float arm_dy = std::sin(angle) * arm_length;
        const float left_hinge = center_x - 1.0F;
        const float right_hinge = center_x + 1.0F;
        const Vec3 left_end{left_hinge + arm_dx, hinge_y + arm_dy, z - 0.04F};
        const Vec3 right_end{right_hinge - arm_dx, hinge_y + arm_dy, z - 0.04F};

        add_line({left_hinge, hinge_y, z - 0.04F}, left_end, active);
        add_line({right_hinge, hinge_y, z - 0.04F}, right_end, active);
        add_hinge(left_hinge, hinge_y, z - 0.08F, amber);
        add_hinge(right_hinge, hinge_y, z - 0.08F, amber);
        add_line({left_hinge, hinge_y + 0.25F, z - 0.05F},
                 {left_hinge, hinge_y - 0.25F, z - 0.05F}, mechanism);
        add_line({right_hinge, hinge_y + 0.25F, z - 0.05F},
                 {right_hinge, hinge_y - 0.25F, z - 0.05F}, mechanism);

        if (deploy > 0.72F) {
            add_line({center_x - 1.12F, hinge_y, z - 0.09F},
                     {center_x + 1.12F, hinge_y, z - 0.09F}, active);
        }

        const Color floor_warning = approach > 5.0F ? amber : danger;
        add_line({center_x - 0.72F, 0.08F, z - 1.15F},
                 {center_x, 0.08F, z - 0.55F}, floor_warning);
        add_line({center_x + 0.72F, 0.08F, z - 1.15F},
                 {center_x, 0.08F, z - 0.55F}, floor_warning);
    }

    void add_bit_chasm(
        const RunnerObstacle& obstacle,
        float approach,
        float progress) {
        constexpr Color void_color{0.003F, 0.006F, 0.018F};
        constexpr Color plate_color{0.012F, 0.045F, 0.060F};
        constexpr Color cyan{0.04F, 0.54F, 0.63F};
        constexpr Color amber{1.0F, 0.43F, 0.08F};
        constexpr Color danger{1.0F, 0.15F, 0.30F};
        constexpr float left_edge = -1.45F;
        constexpr float right_edge = 6.45F;
        const float center_x = 2.5F;
        const float z = 2.5F + obstacle.distance;
        const bool anchored = game_.state().on_chasm_plate &&
            game_.state().active_chasm_obstacle_id == obstacle.id;
        const float plate_progress = anchored
            ? std::clamp(game_.state().chasm_plate_elapsed /
                game_.config().chasm_plate_duration, 0.0F, 1.0F)
            : 0.0F;
        const float collapse = anchored
            ? 1.0F
            : smooth_step(0.18F, 0.95F, progress);
        const float spread = progress * 0.11F;
        const Color edge = mix_color(cyan, danger,
            anchored ? plate_progress : smooth_step(0.55F, 1.0F, progress));

        // This is a full-corridor gap, not a lane-blocking floor tile.
        add_quad(
            left_edge, 0.035F, z - 1.35F,
            right_edge, 0.035F, z + 1.35F,
            void_color);

        add_line({left_edge, 0.07F, z - 1.35F},
                 {left_edge, 0.07F, z + 1.35F}, edge);
        add_line({right_edge, 0.07F, z - 1.35F},
                 {right_edge, 0.07F, z + 1.35F}, edge);
        add_line({left_edge, 0.07F, z - 1.35F},
                 {left_edge + 0.75F, 0.07F, z - 1.35F}, amber);
        add_line({right_edge - 0.75F, 0.07F, z + 1.35F},
                 {right_edge, 0.07F, z + 1.35F}, amber);

        constexpr int columns = 8;
        constexpr int rows = 3;
        const float tile_width = (right_edge - left_edge) /
            static_cast<float>(columns);
        for (int column = 0; column < columns; ++column) {
            for (int row = 0; row < rows; ++row) {
                const float parity = ((column + row) % 2 == 0) ? 1.0F : -1.0F;
                const float tile_x = left_edge +
                    (static_cast<float>(column) + 0.5F) * tile_width +
                    parity * spread;
                const float tile_z = z +
                    (static_cast<float>(row) - 1.0F) * 0.82F +
                    parity * spread * 0.65F;
                const float tile_fall = collapse *
                    (0.42F + static_cast<float>((column + row) % 3) * 0.10F);
                const float tile_y = 0.045F - tile_fall;
                const float half_width = tile_width * 0.43F;
                const float half_depth = 0.34F;
                add_quad(
                    tile_x - half_width, tile_y, tile_z - half_depth,
                    tile_x + half_width, tile_y, tile_z + half_depth,
                    plate_color);

                const Color tile_edge = anchored && collapse < 0.4F
                    ? amber : edge;
                add_line({tile_x - half_width, tile_y, tile_z - half_depth},
                         {tile_x + half_width, tile_y, tile_z - half_depth}, tile_edge);
                add_line({tile_x + half_width, tile_y, tile_z - half_depth},
                         {tile_x + half_width, tile_y, tile_z + half_depth}, tile_edge);
                add_line({tile_x + half_width, tile_y, tile_z + half_depth},
                         {tile_x - half_width, tile_y, tile_z + half_depth}, tile_edge);
                add_line({tile_x - half_width, tile_y, tile_z + half_depth},
                         {tile_x - half_width, tile_y, tile_z - half_depth}, tile_edge);
            }
        }

        if (anchored) {
            const float anchor_x = 2.5F +
                static_cast<float>(game_.state().active_chasm_lane - 1) * 2.65F;
            const float half_width = 0.92F;
            const Color anchor_edge = mix_color(amber, danger, plate_progress);
            const Color anchor_fill = mix_color(plate_color,
                Color{0.10F, 0.075F, 0.035F}, plate_progress * 0.55F);

            // The one tile under the player is held in place while its perimeter
            // fractures. The shrinking timer is mirrored by the red edge glow.
            add_quad(
                anchor_x - half_width, 0.17F, z - 1.00F,
                anchor_x + half_width, 0.17F, z + 1.00F,
                anchor_fill);
            add_line({anchor_x - half_width, 0.18F, z - 1.00F},
                     {anchor_x + half_width, 0.18F, z - 1.00F}, anchor_edge);
            add_line({anchor_x + half_width, 0.18F, z - 1.00F},
                     {anchor_x + half_width, 0.18F, z + 1.00F}, anchor_edge);
            add_line({anchor_x + half_width, 0.18F, z + 1.00F},
                     {anchor_x - half_width, 0.18F, z + 1.00F}, anchor_edge);
            add_line({anchor_x - half_width, 0.18F, z + 1.00F},
                     {anchor_x - half_width, 0.18F, z - 1.00F}, anchor_edge);

            const float crack = 0.18F + plate_progress * 0.42F;
            add_line({anchor_x - crack, 0.19F, z - 0.45F},
                     {anchor_x + 0.05F, 0.19F, z - 0.05F}, danger);
            add_line({anchor_x + 0.05F, 0.19F, z - 0.05F},
                     {anchor_x - 0.02F, 0.19F, z + 0.48F}, danger);
            add_line({anchor_x + 0.18F, 0.19F, z + 0.05F},
                     {anchor_x + crack, 0.19F, z + 0.35F}, danger);

            // A compact vector frame marks the selected anchor tile.
            add_line({anchor_x - 0.20F, 0.24F, z - 0.16F},
                     {anchor_x, 0.50F, z}, anchor_edge);
            add_line({anchor_x, 0.50F, z},
                     {anchor_x + 0.20F, 0.24F, z + 0.16F}, anchor_edge);
            add_line({anchor_x + 0.20F, 0.24F, z + 0.16F},
                     {anchor_x, 0.24F, z + 0.32F}, anchor_edge);
            add_line({anchor_x, 0.24F, z + 0.32F},
                     {anchor_x - 0.20F, 0.24F, z - 0.16F}, anchor_edge);
        }

        // Deterministic data fragments continue falling around the anchored plate.
        for (int fragment = 0; fragment < 12; ++fragment) {
            const float lateral = (static_cast<float>(fragment) - 5.5F) * 0.48F;
            const float depth = (static_cast<float>(fragment % 3) - 1.0F) * 0.78F;
            const float drop = collapse *
                (0.35F + static_cast<float>(fragment % 4) * 0.12F);
            const float x = center_x + lateral;
            const float y = 0.02F - drop;
            const float fragment_z = z + depth +
                spread * static_cast<float>(fragment % 2);
            add_line({x - 0.045F, y, fragment_z},
                     {x + 0.045F, y - 0.07F, fragment_z}, amber);
            add_line({x + 0.045F, y - 0.07F, fragment_z},
                     {x + 0.08F, y - 0.12F, fragment_z + 0.05F}, edge);
        }

        if (approach > 5.0F) {
            add_line({left_edge + 0.2F, 0.09F, z - 1.62F},
                     {center_x, 0.09F, z - 1.20F}, amber);
            add_line({right_edge - 0.2F, 0.09F, z - 1.62F},
                     {center_x, 0.09F, z - 1.20F}, amber);
        }
    }

    void add_crash_effect() {
        const float elapsed = game_.state().crash_effect_elapsed;
        const Color flash{0.76F, 0.97F, 1.0F};
        const Color cyan{0.08F, 0.90F, 1.0F};
        const Color magenta{1.0F, 0.12F, 0.48F};
        const Color amber{1.0F, 0.54F, 0.10F};

        if (elapsed <= 0.12F) {
            add_screen_square(0.5F, 0.49F, 0.032F, flash);
            add_screen_line(0.5F, 0.46F, 0.5F, 0.35F, flash);
            add_screen_line(0.5F, 0.44F, 0.455F, 0.39F, flash);
            add_screen_line(0.5F, 0.44F, 0.545F, 0.39F, flash);
            add_screen_line(0.5F, 0.35F, 0.47F, 0.27F, flash);
            add_screen_line(0.5F, 0.35F, 0.53F, 0.27F, flash);
            add_screen_line(0.455F, 0.40F, 0.545F, 0.40F, cyan);
            add_screen_line(0.5F, 0.58F, 0.5F, 0.62F, flash);
            return;
        }

        const float progress = std::clamp((elapsed - 0.12F) / 0.92F, 0.0F, 1.0F);
        const float fade = 1.0F - progress;
        const std::uint64_t base = game_.seed() ^
            (game_.state().collision_obstacle_id * 0x9E3779B97F4A7C15ULL);
        constexpr int fragment_count = 36;
        for (int index = 0; index < fragment_count; ++index) {
            const std::uint64_t key = base ^
                (static_cast<std::uint64_t>(index) * 0xD1B54A32D192ED03ULL);
            const float angle = unit_hash(key) * 2.0F * kPi;
            const float speed = 0.10F + unit_hash(key + 1U) * 0.22F;
            const float initial_x = 0.5F + (unit_hash(key + 2U) - 0.5F) * 0.08F;
            const float initial_y = 0.45F + (unit_hash(key + 3U) - 0.5F) * 0.25F;
            const float x = initial_x + std::cos(angle) * speed * progress;
            const float y = initial_y + std::sin(angle) * speed * progress -
                progress * progress * (0.17F + unit_hash(key + 4U) * 0.10F);
            const float size = (0.003F + unit_hash(key + 5U) * 0.006F) * fade;
            const Color source = index % 3 == 0 ? cyan : (index % 3 == 1 ? magenta : amber);
            const Color color{
                source.r * fade,
                source.g * fade,
                source.b * fade
            };
            add_screen_square(x, y, std::max(size, 0.0005F), color);
        }
    }

    void add_touch_hud(const RunnerTouchOverlay& touch) {
        constexpr Color cyan{0.08F, 0.78F, 0.88F};
        constexpr Color bright{0.28F, 0.96F, 0.98F};
        constexpr Color muted{0.04F, 0.42F, 0.52F};
        constexpr Color amber{1.0F, 0.58F, 0.12F};
        constexpr Color danger{1.0F, 0.18F, 0.30F};
        const float inverse_aspect = height_ / width_;
        const float stick_x = 0.16F + touch.move_x * 0.066F * inverse_aspect;
        const float stick_y = 0.21F + touch.move_y * 0.066F;

        add_screen_circle(0.16F, 0.21F, 0.132F, muted);
        add_screen_circle(0.16F, 0.21F, 0.112F, cyan);
        add_screen_circle(stick_x, stick_y, 0.044F, touch.move_active ? bright : cyan);
        add_screen_line(0.16F - 0.018F * inverse_aspect, 0.21F,
                        0.16F + 0.018F * inverse_aspect, 0.21F, muted);
        add_screen_line(0.16F, 0.192F, 0.16F, 0.228F, muted);

        const Color action_color = touch.action_active
            ? bright
            : (touch.on_chasm_plate
                ? mix_color(amber, danger, 1.0F -
                    std::clamp(touch.plate_time_remaining / 1.15F, 0.0F, 1.0F))
                : (touch.action_available ? bright : cyan));
        add_screen_circle(0.88F, 0.21F, 0.154F, muted);
        add_screen_circle(0.88F, 0.21F, 0.136F, action_color);

        // The paired chevrons read as a short-range blink/anchor action.
        add_screen_line(0.858F, 0.225F, 0.880F, 0.247F, action_color);
        add_screen_line(0.880F, 0.247F, 0.902F, 0.225F, action_color);
        add_screen_line(0.858F, 0.195F, 0.880F, 0.173F, action_color);
        add_screen_line(0.880F, 0.173F, 0.902F, 0.195F, action_color);

        if (touch.on_chasm_plate) {
            const float fraction = std::clamp(
                touch.plate_time_remaining / 1.15F, 0.0F, 1.0F);
            add_screen_arc(
                0.88F, 0.21F, 0.151F,
                -0.5F * kPi,
                -0.5F * kPi + 2.0F * kPi * fraction,
                danger,
                28);
        } else if (touch.action_available) {
            add_screen_arc(0.88F, 0.21F, 0.151F,
                -0.25F * kPi, 1.25F * kPi, bright, 24);
        }
    }

    const RunnerGame& game_;
    float camera_x_;
    float camera_y_;
    float camera_z_;
    float width_;
    float height_;
    float focal_pixels_;
    std::vector<Vertex>& fill_vertices_;
    std::vector<Vertex>& line_vertices_;
};

} // namespace

struct RunnerHazardRenderer::Impl {
    GlApi api{};
    GLuint program = 0U;
    GLuint fill_buffer = 0U;
    GLuint line_buffer = 0U;
    int width = 1280;
    int height = 720;
    bool initialized = false;
    std::vector<Vertex> fill_vertices;
    std::vector<Vertex> line_vertices;
};

RunnerHazardRenderer::RunnerHazardRenderer() : impl_(std::make_unique<Impl>()) {
    impl_->fill_vertices.reserve(512);
    impl_->line_vertices.reserve(8192);
}

RunnerHazardRenderer::~RunnerHazardRenderer() {
    shutdown();
}

bool RunnerHazardRenderer::initialize() {
    if (impl_->initialized) return true;
    if (!impl_->api.load()) return false;

#if defined(__ANDROID__)
    constexpr const char* vertex_source = kVertexShaderEs;
    constexpr const char* fragment_source = kFragmentShaderEs;
#else
    constexpr const char* vertex_source = kVertexShaderDesktop;
    constexpr const char* fragment_source = kFragmentShaderDesktop;
#endif

    const GLuint vertex_shader = compile_shader(impl_->api, kVertexShader, vertex_source);
    if (vertex_shader == 0U) return false;
    const GLuint fragment_shader = compile_shader(impl_->api, kFragmentShader, fragment_source);
    if (fragment_shader == 0U) {
        impl_->api.delete_shader(vertex_shader);
        return false;
    }

    impl_->program = create_program(impl_->api, vertex_shader, fragment_shader);
    impl_->api.delete_shader(vertex_shader);
    impl_->api.delete_shader(fragment_shader);
    if (impl_->program == 0U) return false;

    impl_->api.gen_buffers(1, &impl_->fill_buffer);
    impl_->api.gen_buffers(1, &impl_->line_buffer);
    if (impl_->fill_buffer == 0U || impl_->line_buffer == 0U) {
        shutdown();
        return false;
    }

    impl_->initialized = true;
    resize(impl_->width, impl_->height);
    SDL_Log("XNO Runner hazard renderer ready (OpenGL vector pass).");
    return true;
}

void RunnerHazardRenderer::resize(int width, int height) noexcept {
    impl_->width = std::max(width, 1);
    impl_->height = std::max(height, 1);
}

void RunnerHazardRenderer::render(
    const RunnerGame& game,
    const RunnerTouchOverlay& touch) {
    if (!impl_->initialized) return;

    impl_->fill_vertices.clear();
    impl_->line_vertices.clear();

    SceneBuilder builder(
        game,
        impl_->width,
        impl_->height,
        impl_->fill_vertices,
        impl_->line_vertices);
    builder.build(touch);

    auto& api = impl_->api;
    api.enable(kDepthTest);
    api.depth_func(kLequal);
    api.use_program(impl_->program);

    const auto draw_vertices = [&api](
        GLuint buffer,
        GLenum primitive,
        const std::vector<Vertex>& vertices) {
        if (vertices.empty()) return;
        api.bind_buffer(kArrayBuffer, buffer);
        api.buffer_data(
            kArrayBuffer,
            static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)),
            vertices.data(),
            kDynamicDraw);
        api.enable_vertex_attrib_array(0U);
        api.enable_vertex_attrib_array(1U);
        api.vertex_attrib_pointer(
            0U, 3, kFloat, 0U, static_cast<GLsizei>(sizeof(Vertex)),
            reinterpret_cast<const void*>(offsetof(Vertex, x)));
        api.vertex_attrib_pointer(
            1U, 3, kFloat, 0U, static_cast<GLsizei>(sizeof(Vertex)),
            reinterpret_cast<const void*>(offsetof(Vertex, r)));
        api.draw_arrays(primitive, 0, static_cast<GLsizei>(vertices.size()));
        api.disable_vertex_attrib_array(0U);
        api.disable_vertex_attrib_array(1U);
    };

    draw_vertices(impl_->fill_buffer, kTriangles, impl_->fill_vertices);
    draw_vertices(impl_->line_buffer, kLines, impl_->line_vertices);
    api.disable(kDepthTest);
}

void RunnerHazardRenderer::shutdown() noexcept {
    if (!impl_) return;
    auto& api = impl_->api;
    if (impl_->fill_buffer != 0U && api.delete_buffers != nullptr) {
        api.delete_buffers(1, &impl_->fill_buffer);
        impl_->fill_buffer = 0U;
    }
    if (impl_->line_buffer != 0U && api.delete_buffers != nullptr) {
        api.delete_buffers(1, &impl_->line_buffer);
        impl_->line_buffer = 0U;
    }
    if (impl_->program != 0U && api.delete_program != nullptr) {
        api.delete_program(impl_->program);
        impl_->program = 0U;
    }
    impl_->initialized = false;
}

#undef XNO_GL_CALL

} // namespace xno::runner::platform
