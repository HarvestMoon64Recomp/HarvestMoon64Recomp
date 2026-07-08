#include "harvestmoon64_config.h"
#include "recompui/recompui.h"
#include "recompui/config.h"
#include "recompinput/recompinput.h"
#include "harvestmoon64_support.h"
#include "ultramodern/config.hpp"
#include "librecomp/files.hpp"
#include "librecomp/config.hpp"
#include "util/file.h"
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <algorithm>

#if defined(_WIN32)
#include <Shlobj.h>
#elif defined(__linux__)
#include <unistd.h>
#include <pwd.h>
#elif defined(__APPLE__)
#include "apple/rt64_apple.h"
#endif

static void add_general_options(recomp::config::Config &config) {
    using EnumOptionVector = const std::vector<recomp::config::ConfigOptionEnumOption>;

}

template <typename T = uint32_t>
T get_general_config_enum_value(const std::string& option_id) {
    return static_cast<T>(std::get<uint32_t>(recompui::config::get_general_config().get_option_value(option_id)));
}

template <typename T = uint32_t>
T get_general_config_number_value(const std::string& option_id) {
    return static_cast<T>(std::get<double>(recompui::config::get_general_config().get_option_value(option_id)));
}

template <typename T = uint32_t>
T get_graphics_config_enum_value(const std::string& option_id) {
    return static_cast<T>(std::get<uint32_t>(recompui::config::get_graphics_config().get_option_value(option_id)));
}

static void add_graphics_options(recomp::config::Config &config) {
}

static void add_sound_options(recomp::config::Config &config) {
    config.add_percent_number_option(
        harvestmoon64::configkeys::sound::music_volume,
        "Music Volume",
        "Controls background music volume.",
        100.0f
    );

    config.add_percent_number_option(
        harvestmoon64::configkeys::sound::ambience_volume,
        "Ambience Volume",
        "Controls ambient/environmental sound volume.",
        100.0f
    );
}

template <typename T = uint32_t>
T get_sound_config_number_value(const std::string& option_id) {
    return static_cast<T>(std::get<double>(recompui::config::get_sound_config().get_option_value(option_id)));
}

int harvestmoon64::get_music_volume() {
    return std::clamp(get_sound_config_number_value<int>(harvestmoon64::configkeys::sound::music_volume), 0, 100);
}

int harvestmoon64::get_ambience_volume() {
    return std::clamp(get_sound_config_number_value<int>(harvestmoon64::configkeys::sound::ambience_volume), 0, 100);
}

static void set_control_defaults() {
    using namespace recompinput;

    // TODO default controls overrides
}

static void set_control_descriptions() {
    recompinput::set_game_input_description(recompinput::GameInput::Y_AXIS_POS, "Move Down / Menu Down");
    recompinput::set_game_input_description(recompinput::GameInput::Y_AXIS_NEG, "Move Up / Menu Up");
    recompinput::set_game_input_description(recompinput::GameInput::X_AXIS_NEG, "Move Left / Menu Left");
    recompinput::set_game_input_description(recompinput::GameInput::X_AXIS_POS, "Move Right / Menu Right");

    recompinput::set_game_input_description(recompinput::GameInput::A, "Confirm / Talk / Interact");
    recompinput::set_game_input_description(recompinput::GameInput::B, "Use tool");
    recompinput::set_game_input_description(recompinput::GameInput::Z, "Identify Item");
    recompinput::set_game_input_description(recompinput::GameInput::L, "Rotate Camera");
    recompinput::set_game_input_description(recompinput::GameInput::R, "Rotate Camera");
    recompinput::set_game_input_description(recompinput::GameInput::START, "Open Pause Menu");

    recompinput::set_game_input_description(recompinput::GameInput::C_UP, "Stow Item");
    recompinput::set_game_input_description(recompinput::GameInput::C_DOWN, "Consume Item");
    recompinput::set_game_input_description(recompinput::GameInput::C_LEFT, "Whistles for Your Horse");
    recompinput::set_game_input_description(recompinput::GameInput::C_RIGHT, "Whistles for Your Dog");

    recompinput::set_game_input_description(recompinput::GameInput::DPAD_UP, "Unused. Mods may use it for additional features.");
    recompinput::set_game_input_description(recompinput::GameInput::DPAD_DOWN, "Unused. Mods may use it for additional features.");
    recompinput::set_game_input_description(recompinput::GameInput::DPAD_LEFT, "Unused. Mods may use it for additional features.");
    recompinput::set_game_input_description(recompinput::GameInput::DPAD_RIGHT, "Unused. Mods may use it for additional features.");
}

void harvestmoon64::init_config() {
    std::filesystem::path recomp_dir = recompui::file::get_app_folder_path();

    if (!recomp_dir.empty()) {
        std::filesystem::create_directories(recomp_dir);
    }

    recompui::config::GeneralTabOptions general_options{};
    general_options.has_rumble_strength = true;
    general_options.has_gyro_sensitivity = false;
    general_options.has_mouse_sensitivity = false;

    auto &general_config = recompui::config::create_general_tab(general_options);
    add_general_options(general_config);

    auto &graphics_config = recompui::config::create_graphics_tab();
    add_graphics_options(graphics_config);

    set_control_defaults();
    set_control_descriptions();
    recompui::config::create_controls_tab();

    auto &sound_config = recompui::config::create_sound_tab();
    add_sound_options(sound_config);

    recompui::config::create_mods_tab();

    recompui::config::finalize();

}
