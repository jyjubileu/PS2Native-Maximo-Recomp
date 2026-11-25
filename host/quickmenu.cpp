#include "quickmenu.hpp"

#include <algorithm>
#include <cstdlib>
#include <string_view>
#include <utility>

namespace quickmenu {
namespace {

// The font: 5 x 8 pixels per letter, printable ASCII (32 to 126), drawn for this project (no font file, no licence to carry). One letter a
// line, its 8 rows from the top separated by spaces, '#' a lit pixel. Capitals and digits use rows 0 to 6; row 7 is for the tails of
// g, j, p, q, y and the comma.
constexpr const char *kFont[95] = {
    "..... ..... ..... ..... ..... ..... ..... .....",  // space
    "..#.. ..#.. ..#.. ..#.. ..#.. ..... ..#.. .....",  // !
    ".#.#. .#.#. ..... ..... ..... ..... ..... .....",  // "
    ".#.#. .#.#. ##### .#.#. ##### .#.#. .#.#. .....",  // #
    "..#.. .#### #.#.. .###. ..#.# ####. ..#.. .....",  // $
    "##... ##..# ...#. ..#.. .#... #..## ...## .....",  // %
    ".##.. #..#. #.#.. .#... #.#.# #..#. .##.# .....",  // &
    "..#.. ..#.. ..... ..... ..... ..... ..... .....",  // '
    "...#. ..#.. .#... .#... .#... ..#.. ...#. .....",  // (
    ".#... ..#.. ...#. ...#. ...#. ..#.. .#... .....",  // )
    "..... ..#.. #.#.# .###. #.#.# ..#.. ..... .....",  // *
    "..... ..#.. ..#.. ##### ..#.. ..#.. ..... .....",  // +
    "..... ..... ..... ..... ..... .##.. ..#.. .#...",  // ,
    "..... ..... ..... ##### ..... ..... ..... .....",  // -
    "..... ..... ..... ..... ..... .##.. .##.. .....",  // .
    "..... ....# ...#. ..#.. .#... #.... ..... .....",  // /
    ".###. #...# #..## #.#.# ##..# #...# .###. .....",  // 0
    "..#.. .##.. ..#.. ..#.. ..#.. ..#.. .###. .....",  // 1
    ".###. #...# ....# ...#. ..#.. .#... ##### .....",  // 2
    "##### ...#. ..#.. ...#. ....# #...# .###. .....",  // 3
    "...#. ..##. .#.#. #..#. ##### ...#. ...#. .....",  // 4
    "##### #.... ####. ....# ....# #...# .###. .....",  // 5
    "..##. .#... #.... ####. #...# #...# .###. .....",  // 6
    "##### ....# ...#. ..#.. .#... .#... .#... .....",  // 7
    ".###. #...# #...# .###. #...# #...# .###. .....",  // 8
    ".###. #...# #...# .#### ....# ...#. .##.. .....",  // 9
    "..... .##.. .##.. ..... .##.. .##.. ..... .....",  // :
    "..... .##.. .##.. ..... .##.. ..#.. .#... .....",  // ;
    "...#. ..#.. .#... #.... .#... ..#.. ...#. .....",  // <
    "..... ..... ##### ..... ##### ..... ..... .....",  // =
    ".#... ..#.. ...#. ....# ...#. ..#.. .#... .....",  // >
    ".###. #...# ....# ...#. ..#.. ..... ..#.. .....",  // ?
    ".###. #...# ....# .##.# #.#.# #.#.# .###. .....",  // @
    ".###. #...# #...# ##### #...# #...# #...# .....",  // A
    "####. #...# #...# ####. #...# #...# ####. .....",  // B
    ".###. #...# #.... #.... #.... #...# .###. .....",  // C
    "####. #...# #...# #...# #...# #...# ####. .....",  // D
    "##### #.... #.... ####. #.... #.... ##### .....",  // E
    "##### #.... #.... ####. #.... #.... #.... .....",  // F
    ".###. #...# #.... #.### #...# #...# .#### .....",  // G
    "#...# #...# #...# ##### #...# #...# #...# .....",  // H
    ".###. ..#.. ..#.. ..#.. ..#.. ..#.. .###. .....",  // I
    "..### ...#. ...#. ...#. ...#. #..#. .##.. .....",  // J
    "#...# #..#. #.#.. ##... #.#.. #..#. #...# .....",  // K
    "#.... #.... #.... #.... #.... #.... ##### .....",  // L
    "#...# ##.## #.#.# #.#.# #...# #...# #...# .....",  // M
    "#...# #...# ##..# #.#.# #..## #...# #...# .....",  // N
    ".###. #...# #...# #...# #...# #...# .###. .....",  // O
    "####. #...# #...# ####. #.... #.... #.... .....",  // P
    ".###. #...# #...# #...# #.#.# #..#. .##.# .....",  // Q
    "####. #...# #...# ####. #.#.. #..#. #...# .....",  // R
    ".#### #.... #.... .###. ....# ....# ####. .....",  // S
    "##### ..#.. ..#.. ..#.. ..#.. ..#.. ..#.. .....",  // T
    "#...# #...# #...# #...# #...# #...# .###. .....",  // U
    "#...# #...# #...# #...# #...# .#.#. ..#.. .....",  // V
    "#...# #...# #...# #.#.# #.#.# #.#.# .#.#. .....",  // W
    "#...# #...# .#.#. ..#.. .#.#. #...# #...# .....",  // X
    "#...# #...# .#.#. ..#.. ..#.. ..#.. ..#.. .....",  // Y
    "##### ....# ...#. ..#.. .#... #.... ##### .....",  // Z
    ".###. .#... .#... .#... .#... .#... .###. .....",  // [
    "..... #.... .#... ..#.. ...#. ....# ..... .....",  // backslash
    ".###. ...#. ...#. ...#. ...#. ...#. .###. .....",  // ]
    "..#.. .#.#. #...# ..... ..... ..... ..... .....",  // ^
    "..... ..... ..... ..... ..... ..... ##### .....",  // _
    ".#... ..#.. ..... ..... ..... ..... ..... .....",  // `
    "..... ..... .###. ....# .#### #...# .#### .....",  // a
    "#.... #.... #.##. ##..# #...# #...# ####. .....",  // b
    "..... ..... .###. #.... #.... #...# .###. .....",  // c
    "....# ....# .##.# #..## #...# #...# .#### .....",  // d
    "..... ..... .###. #...# ##### #.... .###. .....",  // e
    "..##. .#..# .#... ###.. .#... .#... .#... .....",  // f
    "..... ..... .#### #...# #...# .#### ....# .###.",  // g
    "#.... #.... #.##. ##..# #...# #...# #...# .....",  // h
    "..#.. ..... .##.. ..#.. ..#.. ..#.. .###. .....",  // i
    "...#. ..... ..##. ...#. ...#. ...#. #..#. .##..",  // j
    "#.... #.... #..#. #.#.. ##... #.#.. #..#. .....",  // k
    ".##.. ..#.. ..#.. ..#.. ..#.. ..#.. .###. .....",  // l
    "..... ..... ##.#. #.#.# #.#.# #.#.# #.#.# .....",  // m
    "..... ..... #.##. ##..# #...# #...# #...# .....",  // n
    "..... ..... .###. #...# #...# #...# .###. .....",  // o
    "..... ..... ####. #...# #...# ####. #.... #....",  // p
    "..... ..... .#### #...# #...# .#### ....# ....#",  // q
    "..... ..... #.##. ##..# #.... #.... #.... .....",  // r
    "..... ..... .###. #.... .###. ....# ####. .....",  // s
    ".#... .#... ###.. .#... .#... .#..# ..##. .....",  // t
    "..... ..... #...# #...# #...# #..## .##.# .....",  // u
    "..... ..... #...# #...# #...# .#.#. ..#.. .....",  // v
    "..... ..... #...# #...# #.#.# #.#.# .#.#. .....",  // w
    "..... ..... #...# .#.#. ..#.. .#.#. #...# .....",  // x
    "..... ..... #...# #...# #...# .#### ....# .###.",  // y
    "..... ..... ##### ...#. ..#.. .#... ##### .....",  // z
    "...#. ..#.. ..#.. .#... ..#.. ..#.. ...#. .....",  // {
    "..#.. ..#.. ..#.. ..#.. ..#.. ..#.. ..#.. .....",  // |
    ".#... ..#.. ..#.. ...#. ..#.. ..#.. .#... .....",  // }
    "..... ..... .#... #.#.# ...#. ..... ..... .....",  // ~
};

constexpr std::uint32_t rgba(int r, int g, int b, int a = 255) {
    return static_cast<std::uint32_t>(r) | static_cast<std::uint32_t>(g) << 8 | static_cast<std::uint32_t>(b) << 16 |
           static_cast<std::uint32_t>(a) << 24;
}
constexpr auto kPanel = rgba(24, 30, 38), kEdge = rgba(69, 86, 102), kHeader = rgba(32, 42, 53), kRow = rgba(30, 38, 47),
               kSelected = rgba(40, 66, 73), kAccent = rgba(126, 228, 206), kText = rgba(237, 244, 250),
               kMuted = rgba(174, 189, 203), kDisabled = rgba(146, 161, 176), kDark = rgba(18, 33, 40);
constexpr Command kActions[] = {Command::defaults, Command::cancel, Command::apply};

struct Letter {
    char base;
    char accent = 0;
};

Letter next_letter(std::string_view &line) {
    const auto first = static_cast<unsigned char>(line.front());
    line.remove_prefix(1);
    if (first < 128) return {static_cast<char>(first)};
    if (first == 0xc3 && !line.empty()) {  // Portuguese accents in UTF-8.
        const auto c = static_cast<unsigned char>(line.front());
        line.remove_prefix(1);
        const bool upper = c < 0xa0;
        switch (c | 0x20) {
        case 0xa0: return {upper ? 'A' : 'a', '\x60'};
        case 0xa1: return {upper ? 'A' : 'a', '\''};
        case 0xa2: return {upper ? 'A' : 'a', '^'};
        case 0xa3: return {upper ? 'A' : 'a', '~'};
        case 0xa7: return {upper ? 'C' : 'c', ','};
        case 0xa9: return {upper ? 'E' : 'e', '\''};
        case 0xaa: return {upper ? 'E' : 'e', '^'};
        case 0xad: return {upper ? 'I' : 'i', '\''};
        case 0xb3: return {upper ? 'O' : 'o', '\''};
        case 0xb4: return {upper ? 'O' : 'o', '^'};
        case 0xb5: return {upper ? 'O' : 'o', '~'};
        case 0xba: return {upper ? 'U' : 'u', '\''};
        default: break;
        }
    }
    while (!line.empty() && (static_cast<unsigned char>(line.front()) & 0xc0) == 0x80) line.remove_prefix(1);
    return {'?'};
}

int letters(std::string_view line) {
    int n = 0;
    while (!line.empty()) {
        next_letter(line);
        ++n;
    }
    return n;
}

struct Canvas {
    Picture &p;
    void fill(int x, int y, int w, int h, std::uint32_t color) {
        const int x0 = std::max(0, x), y0 = std::max(0, y), x1 = std::min(p.width, x + w), y1 = std::min(p.height, y + h);
        if (x0 >= x1 || y0 >= y1) return;
        for (int j = y0; j < y1; ++j) {
            auto row = p.rgba.begin() + static_cast<std::size_t>(j) * p.width;
            std::fill(row + x0, row + x1, color);
        }
    }

    void text(int x, int y, int s, std::string_view line, std::uint32_t color, int max_width) {
        const int count = max_width / (6 * s);
        const bool ellipsis = letters(line) > count;
        for (int i = 0; i < count && !line.empty(); ++i, x += 6 * s) {
            Letter a = next_letter(line);
            if (ellipsis && i >= count - 3) a = {'.'};
            const auto glyph = [&](char c, int dy, int rows) {
                const char *pixels = kFont[(c >= 32 && c <= 126 ? c : '?') - 32];
                for (int row = 0; row < rows; ++row)
                    for (int col = 0; col < 5; ++col)
                        if (pixels[row * 6 + col] == '#') fill(x + col * s, y + (row + dy) * s, s, s, color);
            };
            glyph(a.base, 0, 8);
            switch (a.accent) {
            case '\'': fill(x + 2 * s, y - s, s, s, color); fill(x + 3 * s, y - 2 * s, s, s, color); break;
            case '\x60': fill(x + s, y - 2 * s, s, s, color); fill(x + 2 * s, y - s, s, s, color); break;
            case '^': fill(x + s, y - s, 3 * s, s, color); fill(x + 2 * s, y - 2 * s, s, s, color); break;
            case '~': fill(x + s, y - 2 * s, 2 * s, s, color); fill(x + 3 * s, y - s, s, s, color); break;
            case ',': fill(x + 2 * s, y + 7 * s, s, 2 * s, color); fill(x + s, y + 9 * s, s, s, color); break;
            }
        }
    }
};

}  // namespace

void Menu::open() {
    open_ = true;
    hover_button_ = pressed_button_ = -1;
    set_tab(Tab::graphics);
}

void Menu::set_tab(Tab tab) {
    tab_ = tab;
    reset_selection();
}

void Menu::switch_tab(int step) {
    set_tab(static_cast<Tab>((static_cast<int>(tab_) + step + 3) % 3));
}

void Menu::move(int step) {
    const int n = static_cast<int>(view_.rows.size());
    const int last = view_.settings ? n : n - 1;
    if (last < 0) return;
    selected_ = selected_ > n ? n : selected_;
    selected_ = (selected_ + step + last + 1) % (last + 1);
    if (view_.settings && selected_ == n) selected_ += 2;  // Apply is the first footer action in keyboard/controller navigation.
}

Selection Menu::side(int step) {
    const int n = static_cast<int>(view_.rows.size());
    if (view_.settings && selected_ >= n) {
        selected_ = n + (selected_ - n + step + 3) % 3;
        return {};
    }
    auto choice = choose();
    if (choice.command != Command::none && view_.rows[selected_].adjustable) {
        choice.step = step;
        return choice;
    }
    return {};
}

Selection Menu::choose() const {
    const int n = static_cast<int>(view_.rows.size());
    if (view_.settings && selected_ >= n && selected_ < n + 3) return {kActions[selected_ - n]};
    if (selected_ >= n || !view_.rows[selected_].enabled) return {};
    const Row &row = view_.rows[selected_];
    return {row.command, row.index};
}

bool Menu::select_at(int x, int y) {
    for (int i = 0; i < static_cast<int>(visible_.size()); ++i) {
        if (visible_[i].contains(x, y) && selected_ != top_ + i) {
            selected_ = top_ + i;
            return true;
        }
    }
    return false;
}

bool Menu::pointer_at(int x, int y, bool pressed) {
    if (!medieval_ || !view_.settings) return false;
    int button = -1;
    for (int i = 0; i < 3; ++i)
        if (buttons_[i].contains(x, y)) button = i;
    const bool changed = hover_button_ != button || (pressed && pressed_button_ != button);
    hover_button_ = button;
    if (pressed) pressed_button_ = button;
    return changed;
}

bool Menu::release_pointer() {
    const bool pressed = pressed_button_ >= 0;
    pressed_button_ = -1;
    return pressed;
}

int Menu::place_rows(int maximum) {
    const int n = static_cast<int>(view_.rows.size()), count = std::clamp(maximum, 0, n);
    selected_ = std::clamp(selected_, 0, std::max(0, n + (view_.settings ? 2 : -1)));
    if (count && selected_ < n) top_ = std::clamp(top_, selected_ + 1 - count, selected_);
    top_ = std::clamp(top_, 0, n - count);
    return count;
}

Selection Menu::click(int x, int y) {
    for (int i = 0; i < 3; ++i) {
        if (tabs_[i].contains(x, y)) {
            set_tab(static_cast<Tab>(i));
            return {};
        }
        if (view_.settings && buttons_[i].contains(x, y)) {
            selected_ = static_cast<int>(view_.rows.size()) + i;
            return choose();
        }
    }
    for (int i = 0; i < static_cast<int>(visible_.size()); ++i) {
        const Rect &r = visible_[i];
        if (!r.contains(x, y)) continue;
        selected_ = top_ + i;
        auto choice = choose();
        if (decrease_[i].contains(x, y)) choice.step = -1;
        return choice;
    }
    return {};
}

void Menu::draw(int window_width, int window_height, View view, Picture &out, bool wide_canvas) {
    view_ = std::move(view);
    visible_.clear();
    decrease_.clear();
    tabs_.fill({});
    buttons_.fill({});
    medieval_ = draw_medieval(window_width, window_height, out, wide_canvas);
    if (medieval_) return;
    out = {};
    if (window_width < 2 || window_height < 2) return;
    const int s = std::clamp(std::min(window_width / 464, window_height / 304), 1, 4);
    const int margin = 12 * s, pw = std::min(430 * s, window_width - 2 * std::min(margin, window_width / 5));
    const int ph = std::min(300 * s, window_height - 2 * std::min(margin, window_height / 5));
    const int px = (window_width - pw) / 2, py = (window_height - ph) / 2;
    const int ox = wide_canvas ? 0 : px, oy = wide_canvas ? 0 : py;
    out.width = wide_canvas ? window_width : pw;
    out.height = wide_canvas ? window_height : ph;
    out.x = ox;
    out.y = oy;
    out.rgba.assign(static_cast<std::size_t>(out.width) * out.height, rgba(4, 9, 14, 155));
    Canvas c{out};
    const auto fill = [&](Rect r, std::uint32_t color) { c.fill(r.x - ox, r.y - oy, r.w, r.h, color); };
    const auto text = [&](int x, int y, int scale, std::string_view label, std::uint32_t color, int width) {
        c.text(x - ox, y - oy, scale, label, color, width);
    };
    fill({px, py, pw, ph}, kEdge);
    fill({px + 1, py + 1, pw - 2, ph - 2}, kPanel);
    if (pw < 220 * s || ph < 116 * s) {
        text(px + 8 * s, py + 8 * s, s, "F1  Fechar", kText, pw - 16 * s);
        return;
    }
    const int x = px + margin, w = pw - 2 * margin;
    fill({px + 1, py + 1, pw - 2, 40 * s}, kHeader);
    fill({px + 1, py + 1, 2 * s, 40 * s}, kAccent);
    text(x, py + 9 * s, 2 * s, "Maximo Rex Settings", kText, w);
    text(x, py + 29 * s, s, view_.profile, kMuted, w);

    const int tab_y = py + 46 * s, tab_w = w / 3;
    for (int i = 0; i < 3; ++i) {
        const bool active = i == static_cast<int>(tab_);
        const auto &tab = tabs_[i] = {x + i * tab_w, tab_y, tab_w - 2 * s, 18 * s};
        fill(tab, active ? kSelected : kHeader);
        if (active) fill({tab.x, tab.y + tab.h - s, tab.w, s}, kAccent);
        text(tab.x + 8 * s, tab.y + 5 * s, s, kTabs[i], active ? kAccent : kMuted, tab.w - 10 * s);
    }

    const int footer = py + ph - (view_.settings ? 49 : 28) * s;
    const int n = static_cast<int>(view_.rows.size()), rows_y = tab_y + 24 * s;
    const int row_h = 21 * s, count = place_rows((footer - rows_y - 2 * s) / row_h);
    for (int i = 0; i < count; ++i) {
        const auto &row = view_.rows[i + top_];
        const Rect r{x, rows_y + i * row_h, w - 5 * s, row_h - 2 * s};
        visible_.push_back(r);
        const bool active = i + top_ == selected_;
        fill(r, active ? kSelected : kRow);
        if (active) fill({r.x, r.y, 2 * s, r.h}, kAccent);
        const int label_w = r.w / 2 - 8 * s;
        text(r.x + 8 * s, r.y + 6 * s, s, row.label, row.enabled ? kText : kDisabled, label_w);
        auto value = row.adjustable && row.enabled ? "<  " + row.value + "  >" : row.value;
        const int value_w = std::min(r.w / 2 - 8 * s, 6 * s * letters(value));
        const int value_x = r.x + r.w - 8 * s - value_w;
        text(value_x, r.y + 6 * s, s, value, row.enabled ? kAccent : kMuted, value_w);
        decrease_.push_back(row.adjustable ? Rect{value_x - 4 * s, r.y, 12 * s, r.h} : Rect{});
    }
    if (count < n && count > 0) {
        const int track = count * row_h - 2 * s;
        fill({x + w - 2 * s, rows_y, 2 * s, track}, kHeader);
        fill({x + w - 2 * s, rows_y + track * top_ / n, 2 * s, std::max(2 * s, track * count / n)}, kAccent);
    }

    std::string_view detail = view_.note;
    if (detail.empty() && selected_ < n) detail = view_.rows[selected_].detail;
    text(x, footer, s, detail, view_.note.empty() ? kMuted : kAccent, w);
    if (view_.settings) {
        const int bw = w / 3;
        for (int i = 0; i < 3; ++i) {
            const auto &button = buttons_[i] = {x + i * bw, footer + 13 * s, bw - 2 * s, 17 * s};
            fill(button, i == 2 ? kAccent : kHeader);
            if (selected_ == n + i) fill({button.x, button.y + button.h - s, button.w, s}, i == 2 ? kDark : kAccent);
            text(button.x + 7 * s, button.y + 5 * s, s, kButtons[i], i == 2 ? kDark : kText, button.w - 9 * s);
        }
    }
    text(x, py + ph - 12 * s, s, "Tab / LB RB: abas     Ctrl+Enter / Y: aplicar     F1: fechar", kMuted, w);
}

void Menu::draw_note(int window_width, int window_height, const std::string &text, Picture &out, bool centered) {
    out = {};
    if (window_width <= 0 || window_height <= 0 || text.empty()) return;
    const int s = std::clamp(window_height / 260, 1, 3), pad = 5 * s;
    out.width = std::min(window_width, 2 * pad + letters(text) * 6 * s);
    out.height = std::min(window_height, 2 * pad + 8 * s);
    out.x = centered ? (window_width - out.width) / 2 : std::min(pad, window_width - out.width);
    out.y = centered ? window_height / 20 : std::min(pad, window_height - out.height);
    out.rgba.assign(static_cast<std::size_t>(out.width) * out.height, kPanel);
    Canvas c{out};
    c.fill(0, 0, s, out.height, kAccent);
    c.text(pad, pad, s, text, kText, out.width - 2 * pad);
}


void draw_weapon_wheel(int window_width, int window_height, int selected, int equipped,
                       unsigned available, Picture &out) {
    out = {};
    const int span = std::min(window_width, window_height);
    if (span < 120 || selected < 0 || selected >= 4 || equipped < 0 || equipped >= 4) return;
    const int diameter = std::min(576, span * 3 / 4);
    const int center = diameter / 2;
    const int radius = center - 6;
    const int scale = std::max(1, diameter / 200);
    const std::array colors{rgba(207, 227, 244), rgba(255, 150, 70), rgba(226, 187, 128), rgba(127, 220, 250)};
    const std::array<std::string_view, 4> labels{"PARILLA", "FLAME", "HAMMER OF", "BLEAK FORGE"};
    const int font = diameter >= 380 ? 2 : 1;
    const std::array<std::string_view, 4> captions{"Espada normal", "Espada de fogo", "Martelo normal", "Martelo de gelo"};
    const auto active = [&](int i) { return (available & (1u << i)) != 0; };

    out.width = out.height = diameter;
    out.x = (window_width - diameter) / 2;
    out.y = (window_height - diameter) / 2;
    out.rgba.resize(static_cast<std::size_t>(diameter) * diameter);
    const int inner = radius * 35 / 100, seam = std::max(2, radius / 45);
    for (int y = 0; y < diameter; ++y) {
        const int dy = y - center;
        for (int x = 0; x < diameter; ++x) {
            const int dx = x - center;
            const int r2 = dx * dx + dy * dy;
            if (r2 >= (radius + 6) * (radius + 6)) continue;
            std::uint32_t color = rgba(5, 9, 14, 210);
            if (r2 < inner * inner) {
                color = r2 >= (inner - 2) * (inner - 2) ? rgba(89, 108, 125) : rgba(18, 25, 33, 250);
            } else if (r2 < radius * radius && std::abs(std::abs(dx) - std::abs(dy)) > seam) {
                const int index = std::abs(dx) > std::abs(dy) ? (dx > 0 ? 1 : 2) : (dy > 0 ? 3 : 0);
                if (index == selected && r2 >= (radius - 4) * (radius - 4)) {
                    color = active(index) ? colors[index] : rgba(107, 116, 128);
                } else if (r2 >= (radius - 1) * (radius - 1)) {
                    color = rgba(70, 86, 103, 230);
                } else if (index == selected && active(index)) {
                    color = rgba(43, 59, 73, 245);
                } else {
                    color = active(index) ? rgba(23, 32, 43, 224) : rgba(15, 19, 26, 225);
                }
            }
            out.rgba[static_cast<std::size_t>(y) * diameter + x] = color;
        }
    }

    Canvas canvas{out};
    const auto centered = [&](int x, int y, int font, std::string_view text, std::uint32_t color) {
        const int width = letters(text) * 6 * font;
        canvas.text(x - width / 2, y, font, text, color, width);
    };
    const int offset = radius * 66 / 100;
    const int xs[] = {center, center + offset, center - offset, center};
    const int ys[] = {center - offset, center, center, center + offset};

    for (int i = 0; i < 4; ++i) {
        const int x = xs[i], y = ys[i] - (i == 3 ? 10 : 13) * scale;
        const auto metal = active(i) ? colors[i] : rgba(94, 103, 114);
        const auto bright = active(i) ? rgba(239, 243, 248) : rgba(114, 122, 130);
        const auto shape = [&](int dx, int dy, int w, int h, std::uint32_t c) {
            canvas.fill(x + dx * scale, y + dy * scale, w * scale, h * scale, c);
        };
        if (i < 2) {
            for (int j = 0; j < 6; ++j) shape(-j / 2 - 1, -11 + j, 2 * (j / 2 + 1), 1, metal);
            shape(-3, -5, 6, 13, metal);
            shape(-1, -6, 1, 13, bright);
            shape(-8, 7, 16, 2, metal);
            shape(-2, 9, 4, 7, rgba(143, 107, 76));
            shape(-3, 15, 6, 2, metal);
            if (i == 1) {
                shape(6, -4, 3, 8, metal);
                shape(8, -8, 2, 6, metal);
                shape(-8, -1, 2, 6, metal);
            }
        } else {
            shape(-2, -3, 4, 19, rgba(143, 107, 76));
            shape(-10, -11, 20, 10, metal);
            shape(-8, -13, 16, 2, metal);
            shape(-12, -9, 2, 6, metal);
            shape(10, -9, 2, 6, metal);
            shape(-8, -10, 16, 1, bright);
            shape(-3, 14, 6, 2, metal);
            if (i == 3) {
                shape(-14, -16, 2, 7, bright);
                shape(-16, -13, 6, 1, bright);
                shape(14, -6, 1, 6, metal);
                shape(12, -4, 5, 1, metal);
            }
        }
        const int label_y = y + 20 * scale;
        centered(x, label_y, font, labels[i], metal);
        if (i == 1 || i == 2)
            centered(x, label_y + 9 * font, font, i == 1 ? "TONGUE" : "HEROES", metal);
        if (i == equipped && active(i))
            canvas.fill(x - 3 * scale, label_y + ((i == 1 || i == 2) ? 20 : 11) * font, 6 * scale, 2 * scale, metal);
    }
    centered(center, center - 16, font, active(selected) ? "EQUIPADA" : "BLOQUEADA",
             active(selected) ? rgba(159, 180, 201) : rgba(217, 166, 122));
    centered(center, center + 3, 1, active(selected) ? captions[equipped] : captions[selected],
             rgba(238, 242, 246));
}

}  // namespace quickmenu
