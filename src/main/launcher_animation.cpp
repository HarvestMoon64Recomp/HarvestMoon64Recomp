#include "harvestmoon64_launcher.h"

#include "SDL.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <limits>
#include <vector>

namespace {

enum class InterpolationMethod {
    Linear,
    Smootherstep,
    EaseOutCubic
};

struct KeyframeRot {
    float seconds;
    float deg;
    InterpolationMethod interpolation_method = InterpolationMethod::Linear;
};

struct Keyframe2D {
    float seconds;
    float x;
    float y;
    InterpolationMethod interpolation_method = InterpolationMethod::Linear;
};

struct AnimationData {
    uint32_t keyframe_index = 0;
    uint32_t loop_keyframe_index = std::numeric_limits<uint32_t>::max();
    float seconds = 0.0f;
    InterpolationMethod interpolation_method = InterpolationMethod::Linear;
};

struct AnimatedSvg {
    recompui::Element *svg = nullptr;
    std::vector<Keyframe2D> position_keyframes;
    std::vector<Keyframe2D> scale_keyframes;
    std::vector<KeyframeRot> rotation_keyframes;
    AnimationData position_animation;
    AnimationData scale_animation;
    AnimationData rotation_animation;
    float width = 0.0f;
    float height = 0.0f;
};

struct LauncherContext {
    AnimatedSvg boy_svg;
    AnimatedSvg dog_svg;
    AnimatedSvg logo_svg;
    AnimatedSvg cloud1_svg;
    AnimatedSvg cloud2_svg;
    AnimatedSvg cloud3_svg;
    AnimatedSvg cloud4_svg;
    recompui::Element* cloud_wrapper = nullptr;
    recompui::Element* wrapper = nullptr;
    std::chrono::steady_clock::time_point last_update_time;
    float seconds = 0.0f;
    bool started = false;
    bool options_enabled = false;
    bool animation_skipped = false;
    std::atomic<bool> skip_animation_next_update = false;
} launcher_context;
constexpr float options_fade_start = 2.6f;
constexpr float options_fade_length = 0.65f;
constexpr float animation_skip_time = options_fade_start + options_fade_length;
constexpr float launcher_stage_width = 1600.0f;
constexpr float launcher_stage_height = 900.0f;

float interpolate_value(float a, float b, float t, InterpolationMethod method) {
    switch (method) {
    case InterpolationMethod::EaseOutCubic:
        return a + (b - a) * (1.0f - ((1.0f - t) * (1.0f - t) * (1.0f - t)));
    case InterpolationMethod::Smootherstep:
        return a + (b - a) * (t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f));
    case InterpolationMethod::Linear:
    default:
        return a + (b - a) * t;
    }
}

void calculate_rot_from_keyframes(const std::vector<KeyframeRot> &kf, AnimationData &an, float delta_time, float &deg) {
    if (kf.empty()) {
        return;
    }

    an.seconds += delta_time;

    while ((an.keyframe_index < (kf.size() - 1)) && (an.seconds >= kf[an.keyframe_index + 1].seconds)) {
        an.keyframe_index++;
    }

    if ((an.loop_keyframe_index != std::numeric_limits<uint32_t>::max()) && (an.keyframe_index >= (kf.size() - 1))) {
        an.seconds = kf[an.loop_keyframe_index].seconds + (an.seconds - kf[an.keyframe_index].seconds);
        an.keyframe_index = an.loop_keyframe_index;
    }

    if (an.keyframe_index >= (kf.size() - 1)) {
        deg = kf[an.keyframe_index].deg;
    }
    else {
        const float t = (an.seconds - kf[an.keyframe_index].seconds) / (kf[an.keyframe_index + 1].seconds - kf[an.keyframe_index].seconds);
        deg = interpolate_value(kf[an.keyframe_index].deg, kf[an.keyframe_index + 1].deg, t, kf[an.keyframe_index + 1].interpolation_method);
    }
}

void calculate_2d_from_keyframes(const std::vector<Keyframe2D> &kf, AnimationData &an, float delta_time, float &x, float &y) {
    if (kf.empty()) {
        return;
    }

    an.seconds += delta_time;

    while ((an.keyframe_index < (kf.size() - 1)) && (an.seconds >= kf[an.keyframe_index + 1].seconds)) {
        an.keyframe_index++;
    }

    if ((an.loop_keyframe_index != std::numeric_limits<uint32_t>::max()) && (an.keyframe_index >= (kf.size() - 1))) {
        an.seconds = kf[an.loop_keyframe_index].seconds + (an.seconds - kf[an.keyframe_index].seconds);
        an.keyframe_index = an.loop_keyframe_index;
    }

    if (an.keyframe_index >= (kf.size() - 1)) {
        x = kf[an.keyframe_index].x;
        y = kf[an.keyframe_index].y;
    }
    else {
        const float t = (an.seconds - kf[an.keyframe_index].seconds) / (kf[an.keyframe_index + 1].seconds - kf[an.keyframe_index].seconds);
        x = interpolate_value(kf[an.keyframe_index].x, kf[an.keyframe_index + 1].x, t, kf[an.keyframe_index + 1].interpolation_method);
        y = interpolate_value(kf[an.keyframe_index].y, kf[an.keyframe_index + 1].y, t, kf[an.keyframe_index + 1].interpolation_method);
    }
}

AnimatedSvg create_animated_svg(recompui::ContextId context, recompui::Element *parent, const std::string &svg_path, float width, float height) {
    AnimatedSvg animated_svg;
    animated_svg.width = width;
    animated_svg.height = height;
    animated_svg.svg = context.create_element<recompui::Svg>(parent, svg_path);
    animated_svg.svg->set_position(recompui::Position::Absolute);
    animated_svg.svg->set_width(width, recompui::Unit::Dp);
    animated_svg.svg->set_height(height, recompui::Unit::Dp);
    return animated_svg;
}

void update_animated_svg(AnimatedSvg &animated_svg, float delta_time, float bg_width, float bg_height) {
    float position_x = 0.0f;
    float position_y = 0.0f;
    float scale_x = 1.0f;
    float scale_y = 1.0f;
    float rotation_degrees = 0.0f;

    calculate_2d_from_keyframes(animated_svg.position_keyframes, animated_svg.position_animation, delta_time, position_x, position_y);
    calculate_2d_from_keyframes(animated_svg.scale_keyframes, animated_svg.scale_animation, delta_time, scale_x, scale_y);
    calculate_rot_from_keyframes(animated_svg.rotation_keyframes, animated_svg.rotation_animation, delta_time, rotation_degrees);

    animated_svg.svg->set_translate_2D(position_x + bg_width / 2.0f - animated_svg.width / 2.0f, position_y + bg_height / 2.0f - animated_svg.height / 2.0f);
    animated_svg.svg->set_scale_2D(scale_x, scale_y);
    animated_svg.svg->set_rotation(rotation_degrees);
}

bool check_skip_input(SDL_Event *event) {
    switch (event->type) {
    case SDL_KEYDOWN:
        return event->key.keysym.scancode == SDL_SCANCODE_ESCAPE ||
            event->key.keysym.scancode == SDL_SCANCODE_SPACE ||
            ((event->key.keysym.scancode == SDL_SCANCODE_RETURN) && ((event->key.keysym.mod & (KMOD_LALT | KMOD_RALT)) == KMOD_NONE));
    case SDL_CONTROLLERBUTTONDOWN:
    case SDL_MOUSEBUTTONDOWN:
        return true;
    default:
        return false;
    }
}

int launcher_event_watch(void *userdata, SDL_Event *event) {
    (void)userdata;

    if (!launcher_context.animation_skipped && check_skip_input(event)) {
        launcher_context.animation_skipped = true;
        launcher_context.skip_animation_next_update = true;
        return 0;
    }

    return 1;
}

void reset_launcher_context() {
    launcher_context.boy_svg = {};
    launcher_context.dog_svg = {};
    launcher_context.logo_svg = {};
    launcher_context.cloud1_svg = {};
    launcher_context.cloud2_svg = {};
    launcher_context.cloud3_svg = {};
    launcher_context.cloud4_svg = {};
    launcher_context.cloud_wrapper = nullptr;
    launcher_context.wrapper = nullptr;
    launcher_context.wrapper = nullptr;
    launcher_context.last_update_time = {};
    launcher_context.seconds = 0.0f;
    launcher_context.started = false;
    launcher_context.options_enabled = false;
    launcher_context.animation_skipped = false;
    launcher_context.skip_animation_next_update = false;
}
void set_options_visible(recompui::LauncherMenu *menu, float phase, bool enabled) {
    phase = std::clamp(phase, 0.0f, 1.0f);

    auto *options_menu = menu->get_game_options_menu();
    options_menu->set_opacity(phase);

    for (auto *option : options_menu->get_options()) {
        option->set_enabled(enabled);
        option->set_opacity(phase);
    }
}

}

void setup_cloud_loop(AnimatedSvg& cloud, float y, float phase_seconds, float cycle_seconds, float offscreen_padding) {
    const float start_x = launcher_stage_width / 2.0f + cloud.width / 2.0f + offscreen_padding;

    const float end_x = -launcher_stage_width / 2.0f - cloud.width / 2.0f - offscreen_padding;

    cloud.position_keyframes = {
        { 0.00f, start_x, y },
        { cycle_seconds, end_x, y, InterpolationMethod::Linear },
    };

    cloud.position_animation = {};
    cloud.position_animation.loop_keyframe_index = 0;

    cloud.position_animation.seconds = phase_seconds;
}

void harvestmoon64::launcher_animation_setup(recompui::LauncherMenu* menu) {
    auto context = recompui::get_current_context();
    recompui::Element* background_container = menu->get_background_container();

    reset_launcher_context();

    background_container->set_background_color({ 0x9D, 0xD7, 0xF5, 0xFF });
    menu->remove_default_title();

    launcher_context.cloud_wrapper = context.create_element<recompui::Element>(background_container, 0);
    launcher_context.cloud_wrapper->set_position(recompui::Position::Absolute);
    launcher_context.cloud_wrapper->set_left(50.0f, recompui::Unit::Percent);
    launcher_context.cloud_wrapper->set_top(50.0f, recompui::Unit::Percent);
    launcher_context.cloud_wrapper->set_width(launcher_stage_width, recompui::Unit::Dp);
    launcher_context.cloud_wrapper->set_height(launcher_stage_height, recompui::Unit::Dp);
    launcher_context.cloud_wrapper->set_translate_2D(-50.0f, -50.0f, recompui::Unit::Percent);

    launcher_context.cloud1_svg = create_animated_svg(context, launcher_context.cloud_wrapper, "Cloud1.svg", 300.0f, 120.0f);
    launcher_context.cloud2_svg = create_animated_svg(context, launcher_context.cloud_wrapper, "Cloud2.svg", 220.0f, 90.0f);
    launcher_context.cloud3_svg = create_animated_svg(context, launcher_context.cloud_wrapper, "Cloud1.svg", 260.0f, 104.0f);
    launcher_context.cloud4_svg = create_animated_svg(context, launcher_context.cloud_wrapper, "Cloud2.svg", 250.0f, 102.0f);

    auto* background_svg = menu->set_launcher_background_svg("BG.svg");
    background_svg->set_left(0.0f);
    background_svg->set_top(0.0f);
    background_svg->set_width(100.0f, recompui::Unit::Percent);
    background_svg->set_height(100.0f, recompui::Unit::Percent);
    background_svg->set_translate_2D(0.0f, 0.0f);

    launcher_context.wrapper = context.create_element<recompui::Element>(background_container, 0);
    launcher_context.wrapper->set_position(recompui::Position::Absolute);
    launcher_context.wrapper->set_left(50.0f, recompui::Unit::Percent);
    launcher_context.wrapper->set_top(50.0f, recompui::Unit::Percent);
    launcher_context.wrapper->set_width(launcher_stage_width, recompui::Unit::Dp);
    launcher_context.wrapper->set_height(launcher_stage_height, recompui::Unit::Dp);
    launcher_context.wrapper->set_translate_2D(-50.0f, -50.0f, recompui::Unit::Percent);

    launcher_context.dog_svg = create_animated_svg(context, launcher_context.wrapper, "Dog.svg", 190.0f, 111.4f);
    launcher_context.boy_svg = create_animated_svg(context, launcher_context.wrapper, "Boy.svg", 250.0f, 264.6f);
    launcher_context.logo_svg = create_animated_svg(context, launcher_context.wrapper, "Logo.svg", 650.0f, 279.5f);




const float cloud_offscreen_padding = 200.0f;
    const float cloud_cycle_seconds = 24.0f;
    const float cloud_spacing = cloud_cycle_seconds / 4.0f;

    setup_cloud_loop(launcher_context.cloud1_svg, -360.0f, cloud_spacing * 0.0f, cloud_cycle_seconds, cloud_offscreen_padding);
    setup_cloud_loop(launcher_context.cloud2_svg, -300.0f, cloud_spacing * 1.0f, cloud_cycle_seconds, cloud_offscreen_padding);
    setup_cloud_loop(launcher_context.cloud3_svg, -390.0f, cloud_spacing * 2.0f, cloud_cycle_seconds, cloud_offscreen_padding);
    setup_cloud_loop(launcher_context.cloud4_svg, -330.0f, cloud_spacing * 3.0f, cloud_cycle_seconds, cloud_offscreen_padding);

    launcher_context.boy_svg.position_keyframes = {
        { 0.00f, -1400.0f, 122.0f },
        { 2.08f, -420.0f, 122.0f, InterpolationMethod::EaseOutCubic },
        { 2.54f, -420.0f, 116.0f, InterpolationMethod::Smootherstep },
        { 3.00f, -420.0f, 122.0f, InterpolationMethod::Smootherstep },
        { 3.46f, -420.0f, 128.0f, InterpolationMethod::Smootherstep },
        { 3.92f, -420.0f, 122.0f, InterpolationMethod::Smootherstep },
    };
    launcher_context.boy_svg.position_animation.loop_keyframe_index = 1;
    launcher_context.boy_svg.rotation_keyframes = {
        { 0.00f, 0.0f },
        { 2.08f, 0.0f, InterpolationMethod::Smootherstep },
        { 2.54f, 2.0f, InterpolationMethod::Smootherstep },
        { 3.00f, 0.0f, InterpolationMethod::Smootherstep },
        { 3.46f, -2.0f, InterpolationMethod::Smootherstep },
        { 3.92f, 0.0f, InterpolationMethod::Smootherstep },
    };
    launcher_context.boy_svg.rotation_animation.loop_keyframe_index = 1;

    launcher_context.dog_svg.position_keyframes = {
        { 0.00f, -1660.0f, 250.0f },
        { 2.08f, -710.0f, 250.0f, InterpolationMethod::EaseOutCubic },
        { 2.54f, -710.0f, 244.0f, InterpolationMethod::Smootherstep },
        { 3.00f, -710.0f, 250.0f, InterpolationMethod::Smootherstep },
        { 3.46f, -710.0f, 256.0f, InterpolationMethod::Smootherstep },
        { 3.92f, -710.0f, 250.0f, InterpolationMethod::Smootherstep },
    };
    launcher_context.dog_svg.position_animation.loop_keyframe_index = 1;
    launcher_context.dog_svg.rotation_keyframes = {
        { 0.00f, 0.0f },
        { 2.08f, 0.0f, InterpolationMethod::Smootherstep },
        { 2.54f, -2.0f, InterpolationMethod::Smootherstep },
        { 3.00f, 0.0f, InterpolationMethod::Smootherstep },
        { 3.46f, 2.0f, InterpolationMethod::Smootherstep },
        { 3.92f, 0.0f, InterpolationMethod::Smootherstep },
    };
    launcher_context.dog_svg.rotation_animation.loop_keyframe_index = 1;

launcher_context.logo_svg.position_keyframes = {
        { 0.00f, 12.0f, -820.0f },
        { 0.48f, 12.0f, -820.0f },
        { 1.26f, 12.0f, -310.0f, InterpolationMethod::Smootherstep },
        { 1.56f, 12.0f, -302.0f, InterpolationMethod::Smootherstep },
        { 1.82f, 12.0f, -330.0f, InterpolationMethod::Smootherstep },

        { 2.28f, 12.0f, -302.0f, InterpolationMethod::Smootherstep },
        { 3.80f, 12.0f, -278.0f, InterpolationMethod::Smootherstep },
        { 5.32f, 12.0f, -302.0f, InterpolationMethod::Smootherstep },
    };

    launcher_context.logo_svg.position_animation.loop_keyframe_index = 5;

    launcher_context.logo_svg.position_animation.interpolation_method = InterpolationMethod::Smootherstep;
    launcher_context.logo_svg.scale_keyframes = {
        { 0.00f, 1.0f, 1.0f },
        { 1.26f, 1.0f, 1.0f },
        { 1.56f, 1.04f, 1.04f },
        { 1.82f, 1.0f, 1.0f },
    };
    launcher_context.logo_svg.scale_animation.interpolation_method = InterpolationMethod::Smootherstep;

    SDL_DelEventWatch(launcher_event_watch, nullptr);
    SDL_AddEventWatch(launcher_event_watch, nullptr);
}

void harvestmoon64::launcher_animation_update(recompui::LauncherMenu *menu) {
    const auto now = std::chrono::steady_clock::now();

    if (!launcher_context.started) {
        launcher_context.last_update_time = now;
        launcher_context.started = true;
    }

    float delta_time = std::chrono::duration<float>(now - launcher_context.last_update_time).count();
    launcher_context.last_update_time = now;

    if (launcher_context.skip_animation_next_update.exchange(false)) {
        delta_time = std::max(0.0f, animation_skip_time - launcher_context.seconds);
    }

    launcher_context.seconds += delta_time;

    update_animated_svg(launcher_context.cloud1_svg, delta_time, launcher_stage_width, launcher_stage_height);
    update_animated_svg(launcher_context.cloud2_svg, delta_time, launcher_stage_width, launcher_stage_height);
    update_animated_svg(launcher_context.cloud3_svg, delta_time, launcher_stage_width, launcher_stage_height);
    update_animated_svg(launcher_context.cloud4_svg, delta_time, launcher_stage_width, launcher_stage_height);
    update_animated_svg(launcher_context.boy_svg, delta_time, launcher_stage_width, launcher_stage_height);
    update_animated_svg(launcher_context.dog_svg, delta_time, launcher_stage_width, launcher_stage_height);
    update_animated_svg(launcher_context.logo_svg, delta_time, launcher_stage_width, launcher_stage_height);

    const float options_phase = (launcher_context.seconds - options_fade_start) / options_fade_length;
    const bool options_finished = options_phase >= 1.0f;

    set_options_visible(menu, options_phase, options_finished);

    if (options_finished && !launcher_context.options_enabled) {
        launcher_context.options_enabled = true;
        SDL_DelEventWatch(launcher_event_watch, nullptr);
    }
}
