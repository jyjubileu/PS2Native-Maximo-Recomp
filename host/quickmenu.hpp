#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace quickmenu {

enum class Tab { graphics, controls, system };
enum class Command {
    none, resume, save, load, report, look, renderer, scale, sharp_2d, fullscreen, sync, fps, current_field, area_reduction, keys, quit,
    keyboard, gamepad, key_edit, pad_edit, wheel_pad, weapon_switch, deadzone, sensitivity, left_reference_gain, invert, rumble, reset_keys, reset_pad,
    apply, cancel, defaults, anti_aliasing, cas, cas_strength
};

struct Row {
    Command command;
    std::string label, value, detail;
    bool adjustable = false;
    bool enabled = true;
    int index = 0;
};

struct View {
    std::vector<Row> rows;
    std::string profile, note;
    bool settings = false;
};

struct Selection {
    Command command = Command::none;
    int index = 0;
    int step = 1;
};

// Straight RGBA, red in the low byte, positioned in physical window pixels.
struct Picture {
    std::vector<std::uint32_t> rgba;
    int width = 0, height = 0, x = 0, y = 0;
};

void draw_weapon_wheel(int window_width, int window_height, int selected, int equipped,
                       unsigned available, Picture &out);

class Menu {
public:
    bool is_open() const { return open_; }
    void open();
    void close() { open_ = false; }
    Tab tab() const { return tab_; }
    void switch_tab(int step);
    void reset_selection() { selected_ = top_ = 0; }
    void move(int step);
    Selection side(int step);
    Selection choose() const;
    bool select_at(int x, int y);
    bool pointer_at(int x, int y, bool pressed);
    bool release_pointer();
    Selection click(int x, int y);
    void draw(int window_width, int window_height, View view, Picture &out, bool wide_canvas = true);

    // Short-lived notices use the same canvas as the menu.
    static void draw_note(int window_width, int window_height, const std::string &text, Picture &out,
                          bool centered = true);

private:
    struct Rect {
        int x = 0, y = 0, w = 0, h = 0;
        bool contains(int px, int py) const { return px >= x && py >= y && px < x + w && py < y + h; }
    };
    void set_tab(Tab tab);
    int place_rows(int maximum);
    bool draw_medieval(int window_width, int window_height, Picture &out, bool wide_canvas);

    static constexpr const char *kTabs[] = {"Gráficos", "Controles", "Sistema"};
    static constexpr const char *kButtons[] = {"Padrões", "Cancelar", "Aplicar"};

    bool open_ = false;
    bool medieval_ = false;
    int hover_button_ = -1, pressed_button_ = -1;
    Tab tab_ = Tab::graphics;
    int selected_ = 0, top_ = 0;
    View view_;
    std::array<Rect, 3> tabs_, buttons_;
    std::vector<Rect> visible_, decrease_;
};

}  // namespace quickmenu
