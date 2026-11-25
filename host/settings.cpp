#include "settings.hpp"

#include "ps2/os.hpp"

#include <algorithm>
#include <charconv>
#include <cstdio>
#include <sstream>
#include <string_view>
#include <utility>

namespace rexsettings {
namespace {

constexpr const char *kRenderers[] = {"auto", "dx12", "vulkan", "cpu"};
constexpr const char *kSync[] = {"auto", "fifo", "immediate"};
constexpr const char *kAntiAliasing[] = {"off", "fxaa", "smaa"};
constexpr const char *kWeaponSwitches[] = {"original", "radial", "dpad"};
constexpr const char *kPadKeys[] = {"select", "l3", "r3", "start", "up", "right", "down", "left",
                                   "l2", "r2", "l1", "r1", "triangle", "circle", "cross", "square"};
constexpr const char *kPadLabels[] = {"Select", "L3", "R3", "Start", "Direcional cima", "Direcional direita", "Direcional baixo",
                                     "Direcional esquerda", "L2", "R2", "L1", "R1", "Triângulo", "Círculo", "X", "Quadrado"};
constexpr const char *kAxes[] = {"invert_lx", "invert_ly", "invert_rx", "invert_ry"};

std::string_view trim(std::string_view s) {
    const auto a = s.find_first_not_of(" \t\r");
    if (a == s.npos) return {};
    return s.substr(a, s.find_last_not_of(" \t\r") - a + 1);
}

bool number(std::string_view s, int min, int max, int &out) {
    if (s.empty()) return false;
    int value = 0;
    const auto [end, error] = std::from_chars(s.data(), s.data() + s.size(), value);
    if (error != std::errc{} || end != s.data() + s.size() || value < min || value > max) return false;
    out = value;
    return true;
}

bool flag(std::string_view s, bool &out) {
    if (s != "on" && s != "off") return false;
    out = s == "on";
    return true;
}

template <class E, std::size_t N>
bool option(std::string_view s, const char *const (&words)[N], E &out) {
    for (std::size_t i = 0; i < N; ++i) {
        if (s != words[i]) continue;
        out = static_cast<E>(i);
        return true;
    }
    return false;
}

bool valid_pad_input(int input) {
    return input >= SDL_CONTROLLER_BUTTON_A && input <= kRightTrigger && input != SDL_CONTROLLER_BUTTON_GUIDE;
}

const char *pad_word(int input) {
    if (input == kLeftTrigger) return "lefttrigger";
    if (input == kRightTrigger) return "righttrigger";
    return SDL_GameControllerGetStringForButton(static_cast<SDL_GameControllerButton>(input));
}

int pad_input(std::string_view word) {
    for (int i = SDL_CONTROLLER_BUTTON_A; i <= kRightTrigger; ++i) {
        if (!valid_pad_input(i)) continue;
        const auto *name = pad_word(i);
        if (name && word == name) return i;
    }
    return -1;
}

bool read_key(Preferences &p, std::string_view key, std::string_view value) {
    for (int action = 0; action < keymap::kChangeableCount; ++action) {
        if (key != keymap::action_name(action)) continue;
        std::vector<int> inputs;
        while (true) {
            if (value.empty()) break;
            const auto comma = value.find(',');
            int input = 0;
            const auto token = trim(value.substr(0, comma));
            if (!number(token, 1, keymap::kWheelDown, input) || keymap::is_reserved(input) ||
                (input >= SDL_NUM_SCANCODES && !(input >= keymap::kMouseLeft && input <= keymap::kMouseX2) &&
                 input != keymap::kWheelUp && input != keymap::kWheelDown)) return false;
            if (std::find(inputs.begin(), inputs.end(), input) == inputs.end()) inputs.push_back(input);
            if (comma == value.npos) break;
            value.remove_prefix(comma + 1);
        }
        p.keys[action] = std::move(inputs);
        return true;
    }
    return false;
}

bool read_pad(Pad &pad, std::string_view key, std::string_view value) {
    if (key == "weapon_wheel") return number(value, -1, SDL_CONTROLLER_BUTTON_MAX - 1, pad.weapon_wheel);
    if (key == "weapon_switch" || key == "l2_mode") return option(value, kWeaponSwitches, pad.weapon_switch);
    if (key == "deadzone") return number(value, 0, 40, pad.deadzone);
    if (key == "sensitivity") return number(value, 50, 150, pad.sensitivity);
    if (key == "left_reference_gain") return flag(value, pad.left_reference_gain);
    if (key == "rumble") return flag(value, pad.rumble);
    for (int i = 0; i < 4; ++i)
        if (key == kAxes[i]) return flag(value, pad.invert[i]);
    for (int bit = 0; bit < kPadButtons; ++bit) {
        if (key != kPadKeys[bit]) continue;
        const int input = pad_input(value);
        if (!valid_pad_input(input)) return false;
        pad.buttons[bit] = input;
        return true;
    }
    return false;
}

bool read_option(Preferences &p, std::string_view key, std::string_view value) {
    if (key == "renderer") return option(value, kRenderers, p.renderer);
    if (key == "sync") return option(value, kSync, p.sync);
    if (key == "scale") return number(value, 0, 4, p.scale);
    if (key == "sharp_2d") return number(value, -1, 2, p.sharp_2d);
    if (key == "look") return number(value, -1, 5, p.look);
    if (key == "fullscreen") return flag(value, p.fullscreen);
    if (key == "fps") return flag(value, p.fps);
    if (key == "current_field") return flag(value, p.current_field);
    if (key == "align_fields") return flag(value, p.align_fields);
    if (key == "motion_fields") return flag(value, p.motion_fields);
    if (key == "dual_motion_fields") return flag(value, p.dual_motion_fields);
    if (key == "anti_aliasing") return option(value, kAntiAliasing, p.anti_aliasing);
    if (key == "cas") return flag(value, p.cas);
    if (key == "cas_strength") {
        int percent = 0;
        if (!number(value, 0, 100, percent)) return false;
        p.cas_strength = (percent + 2) / 5 * 5;
        return true;
    }
    if (key == "area_reduction") return flag(value, p.area_reduction);
    if (key == "default_keys") return flag(value, p.default_keys);
    if (key.starts_with("key.")) return read_key(p, key.substr(4), value);
    if (key.starts_with("pad.")) return read_pad(p.pad, key.substr(4), value);
    return false;
}

}  // namespace

const char *renderer_word(Renderer renderer) { return kRenderers[static_cast<int>(renderer)]; }
const char *sync_word(Sync sync) { return kSync[static_cast<int>(sync)]; }

Preferences load(const std::string &path) {
    Preferences preferences;
    std::FILE *file = ps2::os::open(path, "rb");
    if (!file) return preferences;
    std::string text;
    char buffer[4096];
    for (std::size_t n; (n = std::fread(buffer, 1, sizeof buffer, file)) > 0;) {
        text.append(buffer, n);
        if (text.size() > 64 * 1024) break;
    }
    const bool failed = std::ferror(file) != 0 || text.size() > 64 * 1024;
    std::fclose(file);
    if (failed) {
        std::fprintf(stderr, "[settings] cannot read %s; using defaults\n", path.c_str());
        return preferences;
    }
    if (text.starts_with("\xef\xbb\xbf")) text.erase(0, 3);
    std::istringstream lines(text);
    bool has_legacy_composition = false, has_dual_motion_setting = false;
    int line_number = 0;
    for (std::string line; std::getline(lines, line);) {
        ++line_number;
        const auto content = trim(std::string_view(line).substr(0, line.find('#')));
        if (content.empty()) continue;
        const auto delimiter = content.find('=');
        const auto key = trim(content.substr(0, delimiter));
        if (delimiter == content.npos || !read_option(preferences, key, trim(content.substr(delimiter + 1)))) {
            std::fprintf(stderr, "[settings] %s:%d: ignored invalid setting\n", path.c_str(), line_number);
            continue;
        }
        if (key == "dual_motion_fields") has_dual_motion_setting = true;
        else if (key == "current_field" || key == "align_fields" || key == "motion_fields") has_legacy_composition = true;
    }
    // Keep the selected mode in profiles saved before the dual-motion option existed.
    if (has_legacy_composition && !has_dual_motion_setting) preferences.dual_motion_fields = false;
    auto ordered = preferences.pad.buttons;
    std::sort(ordered.begin(), ordered.end());
    if (std::adjacent_find(ordered.begin(), ordered.end()) != ordered.end()) {
        preferences.pad.buttons = Pad{}.buttons;
        std::fprintf(stderr, "[settings] duplicate pad inputs; using the default pad layout\n");
    }
    if (!weapon_wheel_button(preferences.pad, preferences.pad.weapon_wheel)) preferences.pad.weapon_wheel = -1;
    std::fprintf(stderr, "[settings] loaded %s\n", path.c_str());
    return preferences;
}

bool save(const std::string &path, const Preferences &p, std::string &why) {
    if (path.empty()) {
        why = "settings path is empty";
        return false;
    }
    std::error_code error;
    const auto dir = ps2::os::path(path).parent_path();
    if (!dir.empty()) std::filesystem::create_directories(dir, error);
    if (error) {
        why = "cannot create settings folder: " + error.message();
        return false;
    }
    ps2::os::FileLock lock;
    bool busy = false;
    if (!lock.take(path + ".lock", busy, why)) {
        if (busy) why = "settings are being saved by another process";
        return false;
    }
    std::ostringstream out;
    out << "# Maximo Rex Settings\n"
        << "renderer = " << renderer_word(p.renderer) << '\n'
        << "sync = " << sync_word(p.sync) << '\n'
        << "scale = " << p.scale << '\n'
        << "sharp_2d = " << p.sharp_2d << '\n'
        << "look = " << p.look << '\n'
        << "fullscreen = " << (p.fullscreen ? "on" : "off") << '\n'
        << "fps = " << (p.fps ? "on" : "off") << '\n'
        << "current_field = " << (p.current_field ? "on" : "off") << '\n'
        << "align_fields = " << (p.align_fields ? "on" : "off") << '\n'
        << "motion_fields = " << (p.motion_fields ? "on" : "off") << '\n'
        << "dual_motion_fields = " << (p.dual_motion_fields ? "on" : "off") << '\n'
        << "anti_aliasing = " << kAntiAliasing[static_cast<int>(p.anti_aliasing)] << '\n'
        << "cas = " << (p.cas ? "on" : "off") << '\n'
        << "cas_strength = " << p.cas_strength << '\n'
        << "area_reduction = " << (p.area_reduction ? "on" : "off") << '\n'
        << "default_keys = " << (p.default_keys ? "on" : "off") << '\n';
    if (std::any_of(p.keys.begin(), p.keys.end(), [](const auto &keys) { return keys.has_value(); })) {
        out << "\n# Keyboard overrides (SDL scancodes and mouse codes)\n";
        for (int action = 0; action < keymap::kChangeableCount; ++action) {
            if (!p.keys[action]) continue;
            const auto &keys = *p.keys[action];
            out << "# " << keymap::action_label(action);
            for (const auto input : keys) out << "  " << keymap::input_name(input);
            out << '\n' << "key." << keymap::action_name(action) << " = ";
            for (std::size_t i = 0; i < keys.size(); ++i) out << (i ? ", " : "") << keys[i];
            out << '\n';
        }
    }
    out << "\n# Gamepad inputs after SDL's device mapping\n"
        << "pad.deadzone = " << p.pad.deadzone << '\n'
        << "pad.sensitivity = " << p.pad.sensitivity << '\n'
        << "pad.left_reference_gain = " << (p.pad.left_reference_gain ? "on" : "off") << '\n'
        << "pad.rumble = " << (p.pad.rumble ? "on" : "off") << '\n'
        << "pad.weapon_switch = " << kWeaponSwitches[static_cast<int>(p.pad.weapon_switch)] << '\n';
    if (p.pad.weapon_wheel >= 0) out << "pad.weapon_wheel = " << p.pad.weapon_wheel << '\n';
    for (int i = 0; i < 4; ++i) out << "pad." << kAxes[i] << " = " << (p.pad.invert[i] ? "on" : "off") << '\n';
    for (int bit = 0; bit < kPadButtons; ++bit) out << "pad." << kPadKeys[bit] << " = " << pad_word(p.pad.buttons[bit]) << '\n';
    const auto text = out.str();
    return ps2::os::replace_file(path, text.data(), text.size(), &why);
}

keymap::Config keyboard(const Preferences &p, const keymap::Config &base) {
    auto config = base;
    if (p.default_keys) {
        std::vector<int> changed;
        keymap::restore_default_inputs(config, changed);
    }
    for (int action = 0; action < keymap::kChangeableCount; ++action)
        if (p.keys[action]) config.inputs[action] = *p.keys[action];
    return config;
}

const char *ps2_button_label(int bit) { return kPadLabels[bit]; }

std::string pad_input_name(int input) {
    switch (input) {
    case kLeftTrigger: return "LT";
    case kRightTrigger: return "RT";
    case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: return "LB";
    case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: return "RB";
    case SDL_CONTROLLER_BUTTON_LEFTSTICK: return "L3";
    case SDL_CONTROLLER_BUTTON_RIGHTSTICK: return "R3";
    case SDL_CONTROLLER_BUTTON_DPAD_UP: return "D-pad cima";
    case SDL_CONTROLLER_BUTTON_DPAD_DOWN: return "D-pad baixo";
    case SDL_CONTROLLER_BUTTON_DPAD_LEFT: return "D-pad esquerda";
    case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: return "D-pad direita";
    default: break;
    }
    if (const auto *word = pad_word(input)) {
        std::string name = word;
        if (!name.empty() && name[0] >= 'a' && name[0] <= 'z') name[0] -= 'a' - 'A';
        return name;
    }
    return {};
}

bool weapon_wheel_button(const Pad &pad, int input) {
    if (input < 0 || input >= SDL_CONTROLLER_BUTTON_MAX || input == SDL_CONTROLLER_BUTTON_GUIDE ||
        input == SDL_CONTROLLER_BUTTON_BACK || input == SDL_CONTROLLER_BUTTON_START ||
        input == SDL_CONTROLLER_BUTTON_B) return false;
    return std::find(pad.buttons.begin(), pad.buttons.end(), input) == pad.buttons.end();
}

bool assign_weapon_wheel(Pad &pad, int input, std::string &message) {
    if (!weapon_wheel_button(pad, input)) {
        message = "Botão em uso. Escolha um botão livre.";
        return false;
    }
    pad.weapon_wheel = input;
    message = "Roda de armas configurada. Use Aplicar para salvar.";
    return true;
}

bool assign_pad(Pad &pad, int bit, int input, std::string &message) {
    if (bit < 0 || bit >= kPadButtons || !valid_pad_input(input)) return false;
    Pad draft = pad;
    int &binding = draft.buttons[bit];
    const auto other = std::find(draft.buttons.begin(), draft.buttons.end(), input);
    message = std::string(ps2_button_label(bit)) + " usa " + pad_input_name(input) + ".";
    if (other != draft.buttons.end() && other != draft.buttons.begin() + bit) {
        *other = binding;
        message += " Trocado com " + std::string(ps2_button_label(static_cast<int>(other - draft.buttons.begin()))) + ".";
    }
    binding = input;
    if (draft.weapon_wheel >= 0 && !weapon_wheel_button(draft, draft.weapon_wheel)) {
        message = "Botão reservado para a roda de armas.";
        return false;
    }
    pad = draft;
    return true;
}

std::uint8_t stick_value(int raw, const Pad &pad, int axis) {
    if (pad.invert[axis]) raw = std::clamp(-raw, -32768, 32767);
    const int sensitivity = axis < 2 && pad.left_reference_gain ? kPcsx2StickGain : pad.sensitivity;
    if (pad.deadzone || sensitivity != 100) {
        const int limit = raw < 0 ? 32768 : 32767;
        const int deadzone = limit * pad.deadzone / 100;
        const int distance = std::max(0, std::abs(raw) - deadzone);
        const int scaled = std::min(limit, static_cast<int>(static_cast<double>(distance) / (limit - deadzone) * limit * sensitivity / 100.0));
        raw = raw < 0 ? -scaled : scaled;
    }
    return static_cast<std::uint8_t>((raw + 32768) >> 8);
}

}  // namespace rexsettings
