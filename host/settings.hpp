#pragma once

#include "keymap.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string>

#include <SDL.h>

namespace rexsettings {

enum class Renderer { automatic, d3d12, vulkan, cpu };
enum class Sync { automatic, fifo, immediate };
enum class AntiAliasing { off, fxaa, smaa };
enum class WeaponSwitch { original, radial, dpad };

constexpr int kLeftTrigger = SDL_CONTROLLER_BUTTON_MAX, kRightTrigger = kLeftTrigger + 1;
constexpr int kPadButtons = 16;
constexpr int kPcsx2StickGain = 133;

struct Pad {
    // SDL inputs in the order of the PS2 pad bits.
    std::array<int, kPadButtons> buttons = {
        SDL_CONTROLLER_BUTTON_BACK, SDL_CONTROLLER_BUTTON_LEFTSTICK, SDL_CONTROLLER_BUTTON_RIGHTSTICK, SDL_CONTROLLER_BUTTON_START,
        SDL_CONTROLLER_BUTTON_DPAD_UP, SDL_CONTROLLER_BUTTON_DPAD_RIGHT, SDL_CONTROLLER_BUTTON_DPAD_DOWN, SDL_CONTROLLER_BUTTON_DPAD_LEFT,
        kLeftTrigger, kRightTrigger, SDL_CONTROLLER_BUTTON_LEFTSHOULDER, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,
        SDL_CONTROLLER_BUTTON_Y, SDL_CONTROLLER_BUTTON_B, SDL_CONTROLLER_BUTTON_A, SDL_CONTROLLER_BUTTON_X,
    };
    int weapon_wheel = -1;  // physical button without an assigned game action
    WeaponSwitch weapon_switch = WeaponSwitch::original;
    int deadzone = 0;
    int sensitivity = 100;
    bool left_reference_gain = false;
    std::array<bool, 4> invert{};  // LX, LY, RX, RY.
    bool rumble = true;
};

struct Preferences {
    Renderer renderer = Renderer::automatic;
    Sync sync = Sync::automatic;
    int scale = 0;      // 0 uses the value from keys.ini, then the runtime default.
    int sharp_2d = -1;  // -1 uses the value from keys.ini.
    int look = -1;
    bool fullscreen = false;
    bool fps = false;
    bool current_field = false;
    bool align_fields = false;
    bool motion_fields = false;
    bool dual_motion_fields = true;
    AntiAliasing anti_aliasing = AntiAliasing::off;
    bool cas = false;
    int cas_strength = 30;
    bool area_reduction = false;
    bool default_keys = false;
    std::array<std::optional<std::vector<int>>, keymap::kChangeableCount> keys;
    Pad pad;
};

Preferences load(const std::string &path);
bool save(const std::string &path, const Preferences &preferences, std::string &why);
const char *renderer_word(Renderer renderer);
const char *sync_word(Sync sync);
keymap::Config keyboard(const Preferences &preferences, const keymap::Config &base);
const char *ps2_button_label(int bit);
std::string pad_input_name(int input);
bool assign_weapon_wheel(Pad &pad, int input, std::string &message);
bool weapon_wheel_button(const Pad &pad, int input);
bool assign_pad(Pad &pad, int bit, int input, std::string &message);
std::uint8_t stick_value(int raw, const Pad &pad, int axis);

}  // namespace rexsettings
