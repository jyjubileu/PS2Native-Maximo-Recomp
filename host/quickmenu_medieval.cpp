#include "quickmenu.hpp"

#ifdef _WIN32

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <wincodec.h>

#include "ps2/os.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <string_view>
#include <utility>

namespace quickmenu {
namespace {

struct Box {
    int x, y, w, h;
};
struct Insets { int left, top, right, bottom; };
struct Spec {
    const wchar_t *file;
    int w, h;
    Insets margins;
};

enum class Art {
#define MAXIMO_F1_ASSET(id, file, w, h, l, t, r, b) id,
#include "../assets/ui/maximo_f1/regions.inc"
#undef MAXIMO_F1_ASSET
    count
};
constexpr Spec kSpecs[] = {
#define MAXIMO_F1_ASSET(id, file, w, h, l, t, r, b) {L##file, w, h, {l, t, r, b}},
#include "../assets/ui/maximo_f1/regions.inc"
#undef MAXIMO_F1_ASSET
};
static_assert(std::size(kSpecs) == static_cast<std::size_t>(Art::count));

template<class T> struct Com {
    T *ptr = nullptr;
    ~Com() { if (ptr) ptr->Release(); }
    T *operator->() const { return ptr; }
};

constexpr std::uint32_t color(int r, int g, int b, int a = 255) {
    return std::uint32_t(r) | (std::uint32_t(g) << 8) | (std::uint32_t(b) << 16) | (std::uint32_t(a) << 24);
}

constexpr std::uint32_t premultiply(std::uint32_t pixel) {
    const int a = pixel >> 24;
    return color((int(pixel & 255) * a + 127) / 255, (int((pixel >> 8) & 255) * a + 127) / 255,
                 (int((pixel >> 16) & 255) * a + 127) / 255, a);
}

constexpr auto kScrim = color(8, 10, 13, 170);

struct Theme {
    using Pixels = std::vector<std::uint32_t>;
    std::array<Pixels, std::size(kSpecs)> art;
    bool ready = false;

    Theme() {
        if (ps2::os::env("PS2_MAXIMO_F1_THEME") == "classic") return;
        const auto com = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        if (SUCCEEDED(com) || com == RPC_E_CHANGED_MODE) {
            ready = load();
            if (SUCCEEDED(com)) CoUninitialize();
        }
        std::fprintf(stderr, "%s\n", ready ? "[menu] F1 medieval theme loaded" :
                     "[menu] F1 assets unavailable; using the classic theme");
    }

    bool load() {
        std::array<wchar_t, 32768> exe{};
        const DWORD size = GetModuleFileNameW(nullptr, exe.data(), static_cast<DWORD>(exe.size()));
        if (!size || size == exe.size()) return false;
        const auto folder = std::filesystem::path(std::wstring_view(exe.data(), size)).parent_path() /
                            L"assets" / L"ui" / L"maximo_f1";
        Com<IWICImagingFactory> factory;
        if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                                    IID_IWICImagingFactory, reinterpret_cast<void **>(&factory.ptr)))) return false;
        for (std::size_t i = 0; i < std::size(kSpecs); ++i) {
            const auto &spec = kSpecs[i];
            Com<IWICBitmapDecoder> decoder;
            Com<IWICBitmapFrameDecode> frame;
            Com<IWICFormatConverter> converter;
            const auto path = folder / spec.file;
            UINT w = 0, h = 0;
            if (FAILED(factory->CreateDecoderFromFilename(path.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &decoder.ptr)) ||
                FAILED(decoder->GetFrame(0, &frame.ptr)) || FAILED(frame->GetSize(&w, &h)) ||
                w != UINT(spec.w) || h != UINT(spec.h) || FAILED(factory->CreateFormatConverter(&converter.ptr)) ||
                FAILED(converter->Initialize(frame.ptr, GUID_WICPixelFormat32bppRGBA, WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom)))
                return false;
            art[i].resize(std::size_t(w) * h);
            if (FAILED(converter->CopyPixels(nullptr, 4 * w, 4 * w * h, reinterpret_cast<BYTE *>(art[i].data()))))
                return false;
            for (auto &pixel : art[i]) pixel = premultiply(pixel);
        }
        return true;
    }
};

void blend(std::uint32_t &dst, std::uint32_t src) {
    const unsigned a = src >> 24;
    if (a == 0) return;
    if (a == 255) {
        dst = src;
        return;
    }
    const unsigned inverse = 255 - a;
    std::uint32_t result = 0;
    for (int shift = 0; shift < 32; shift += 8) {
        const auto over = (src >> shift) & 255;
        const auto under = (dst >> shift) & 255;
        result |= (over + (under * inverse + 127) / 255) << shift;
    }
    dst = result;
}

std::uint32_t mix(std::uint32_t a, std::uint32_t b, unsigned weight) {
    std::uint32_t pixel = 0;
    for (int shift = 0; shift < 32; shift += 8) {
        const auto low = (a >> shift) & 255;
        const auto high = (b >> shift) & 255;
        pixel |= ((low * (256 - weight) + high * weight + 128) >> 8) << shift;
    }
    return pixel;
}

std::array<int, 4> stops(int width, int left, int right, double scale) {
    int a = static_cast<int>(std::lround(left * scale));
    int b = static_cast<int>(std::lround(right * scale));
    if (a + b > width) {
        a = width * a / (a + b);
        b = width - a;
    }
    return {0, a, width - b, width};
}

struct Sample { int i, j; unsigned fraction; };

std::vector<Sample> samples(int count, int source, bool mirror = false) {
    std::vector<Sample> axis;
    axis.reserve(count);
    for (int i = 0; i < count; ++i) {
        const double f = std::clamp(((mirror ? count - 1 - i : i) + .5) * source / count - .5, 0.0, double(source - 1));
        const int j = int(f);
        axis.push_back({j, std::min(j + 1, source - 1), unsigned((f - j) * 256)});
    }
    return axis;
}

class Painter {
public:
    Painter(Picture &p, const Theme &theme, double scale, int text_width = 0, int text_height = 0)
        : p_(p), theme_(theme), scale_(scale), text_width_(text_width), text_height_(text_height) {
        if (!text_width || !text_height) return;
        dc_ = CreateCompatibleDC(nullptr);
        if (!dc_) return;
        BITMAPINFO info{};
        info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        info.bmiHeader.biWidth = text_width_;
        info.bmiHeader.biHeight = -text_height_;
        info.bmiHeader.biPlanes = 1;
        info.bmiHeader.biBitCount = 32;
        info.bmiHeader.biCompression = BI_RGB;
        bitmap_ = CreateDIBSection(dc_, &info, DIB_RGB_COLORS, reinterpret_cast<void **>(&mask_), nullptr, 0);
        if (!bitmap_) return;
        previous_ = SelectObject(dc_, bitmap_);
        SetBkMode(dc_, TRANSPARENT);
        SetTextColor(dc_, RGB(255, 255, 255));
    }
    ~Painter() {
        if (dc_) SelectObject(dc_, GetStockObject(DEFAULT_GUI_FONT));
        for (const auto &font : fonts_) if (font.handle) DeleteObject(font.handle);
        if (previous_ && previous_ != HGDI_ERROR) SelectObject(dc_, previous_);
        if (bitmap_) DeleteObject(bitmap_);
        if (dc_) DeleteDC(dc_);
    }
    bool valid() const { return previous_ && previous_ != HGDI_ERROR && font(28, true); }

    void plate(Art id, Box r, bool mirror = false) {
        const auto index = static_cast<std::size_t>(id);
        const auto &spec = kSpecs[index];
        const auto &m = spec.margins;
        const auto source_x = stops(spec.w, m.left, m.right, 1);
        const auto source_y = stops(spec.h, m.top, m.bottom, 1);
        const auto target_x = stops(r.w, m.left, m.right, scale_);
        const auto target_y = stops(r.h, m.top, m.bottom, scale_);
        for (int y = 0; y < 3; ++y) for (int x = 0; x < 3; ++x) {
            const Box from{source_x[x], source_y[y], source_x[x+1] - source_x[x], source_y[y+1] - source_y[y]};
            const Box to{r.x + target_x[x], r.y + target_y[y], target_x[x+1] - target_x[x], target_y[y+1] - target_y[y]};
            if (from.w && from.h && to.w && to.h) raster(theme_.art[index], spec.w, from, to, mirror);
        }
    }

    void fill(Box area, std::uint32_t ink) {
        area.x -= p_.x;
        area.y -= p_.y;
        for (int y = std::max(0, area.y); y < std::min(p_.height, area.y + area.h); ++y) {
            auto *row = p_.rgba.data() + std::size_t(y) * p_.width;
            for (int x = std::max(0, area.x); x < std::min(p_.width, area.x + area.w); ++x) row[x] = ink;
        }
    }

    void text(Box r, int size, std::string_view string, std::uint32_t ink, UINT align = DT_LEFT,
              bool bold = true, bool multiline = false, std::uint32_t lower = 0, int *advance = nullptr) {
        if (string.empty() || r.w <= 0 || r.h <= 0 || r.w > text_width_ || r.h > text_height_) return;
        const int n = MultiByteToWideChar(CP_UTF8, 0, string.data(), int(string.size()), nullptr, 0);
        if (n <= 0) return;
        std::wstring utf16(n, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, string.data(), int(string.size()), utf16.data(), n);
        const auto face = font(size, bold);
        if (!face) return;
        SelectObject(dc_, face);
        if (advance) {
            SIZE extent{};
            if (GetTextExtentPoint32W(dc_, utf16.c_str(), n, &extent)) *advance = std::min(r.w, int(extent.cx));
        }
        for (int y = 0; y < r.h; ++y) std::fill_n(mask_ + std::size_t(y) * text_width_, r.w, 0u);
        RECT box{0, 0, r.w, r.h};
        const UINT flags = DT_NOPREFIX | DT_END_ELLIPSIS | align | (multiline ? DT_WORDBREAK | DT_EDITCONTROL : DT_SINGLELINE | DT_VCENTER);
        if (multiline) {
            RECT measuring = box;
            DrawTextW(dc_, utf16.c_str(), n, &measuring, flags | DT_CALCRECT);
            box.top = std::max(0L, (r.h - measuring.bottom) / 2);
        }
        DrawTextW(dc_, utf16.c_str(), n, &box, flags);
        GdiFlush();
        const int shadow = std::max(1, int(std::round(scale_ * (lower ? 3 : 1))));
        draw_glyphs(r, color(12, 6, 3), 0, shadow);
        draw_glyphs(r, ink, lower, 0);
    }

    void finish() {
        for (auto &pixel : p_.rgba) {
            if (pixel == premultiply(kScrim)) {
                pixel = kScrim;
                continue;
            }
            const unsigned a = pixel >> 24;
            if (!a || a == 255) continue;
            pixel = color(std::min(255u, ((pixel & 255) * 255 + a / 2) / a),
                          std::min(255u, (((pixel >> 8) & 255) * 255 + a / 2) / a),
                          std::min(255u, (((pixel >> 16) & 255) * 255 + a / 2) / a), a);
        }
    }

private:
    struct Font { int size; bool bold; HFONT handle; };

    HFONT font(int size, bool bold) const {
        const int px = std::max(1, int(std::lround(size * scale_)));
        for (const auto &cached : fonts_)
            if (cached.size == px && cached.bold == bold) return cached.handle;
        const auto face = CreateFontW(-px, 0, 0, 0, bold ? FW_BOLD : FW_NORMAL, FALSE, FALSE, FALSE,
                                      DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
                                      VARIABLE_PITCH | FF_ROMAN, L"Georgia");
        fonts_.push_back({px, bold, face});
        return face;
    }

    void raster(const Theme::Pixels &pixels, int stride, Box from, Box to, bool mirror) {
        to.x -= p_.x;
        to.y -= p_.y;
        const auto xaxis = samples(to.w, from.w, mirror);
        const auto yaxis = samples(to.h, from.h);
        const int first_x = std::max(0, -to.x), last_x = std::min(to.w, p_.width - to.x);
        for (int j = std::max(0, -to.y); j < std::min(to.h, p_.height - to.y); ++j) {
            const auto sy = yaxis[j];
            const auto *top = pixels.data() + std::size_t(from.y + sy.i) * stride + from.x;
            const auto *bottom = pixels.data() + std::size_t(from.y + sy.j) * stride + from.x;
            auto *dest = p_.rgba.data() + std::size_t(to.y + j) * p_.width;
            for (int i = first_x; i < last_x; ++i) {
                const auto sx = xaxis[i];
                auto pixel = top[sx.i];
                if (sx.fraction) pixel = mix(pixel, top[sx.j], sx.fraction);
                if (sy.fraction) {
                    auto next = bottom[sx.i];
                    if (sx.fraction) next = mix(next, bottom[sx.j], sx.fraction);
                    pixel = mix(pixel, next, sy.fraction);
                }
                blend(dest[to.x + i], pixel);
            }
        }
    }

    void draw_glyphs(Box r, std::uint32_t top, std::uint32_t lower, int offset) {
        r.x -= p_.x;
        r.y -= p_.y;
        const int x0 = std::max(0, -r.x), x1 = std::min(r.w, p_.width - r.x);
        for (int y = std::max(0, -r.y - offset); y < std::min(r.h, p_.height - r.y - offset); ++y) {
            const auto *source = mask_ + std::size_t(y) * text_width_;
            auto *dest = p_.rgba.data() + std::size_t(r.y + y + offset) * p_.width;
            const auto ink = lower ? mix(top, lower, unsigned(std::clamp((y - r.h * .28) * 450 / r.h, 0.0, 256.0))) : top;
            for (int x = x0; x < x1; ++x) {
                const auto opacity = source[x] & 255;
                if (opacity) blend(dest[r.x + x], premultiply((ink & 0x00ffffff) | (opacity << 24)));
            }
        }
    }

    Picture &p_;
    const Theme &theme_;
    double scale_;
    int text_width_, text_height_;
    HDC dc_ = nullptr;
    HBITMAP bitmap_ = nullptr;
    HGDIOBJ previous_ = nullptr;
    std::uint32_t *mask_ = nullptr;
    mutable std::vector<Font> fonts_;
};

}  // namespace

bool Menu::draw_medieval(int width, int height, Picture &out, bool wide_canvas) {
    const double scale = std::min(width / 1560.0, height / 1080.0);
    if (scale < .36) return false;
    static const Theme theme;
    if (!theme.ready) return false;
    const auto s = [&](int px) { return int(std::lround(px * scale)); };
    constexpr int kW = 1392, kH = 948;
    const int pw = s(kW), ph = s(kH), px = (width - pw) / 2, py = (height - ph) / 2;
    const auto box = [&](int x, int y, int w, int h) -> Box {
        return {px + s(x), py + s(y), s(x + w) - s(x), s(y + h) - s(y)};
    };
    const auto hit = [](Box r) -> Rect { return {r.x, r.y, r.w, r.h}; };
    constexpr auto ivory = color(255, 238, 202), active_text = color(255, 225, 155), muted = color(223, 190, 151);
    constexpr auto disabled = color(182, 156, 133), brass = color(177, 126, 69);
    constexpr int body_x = 48, body_w = kW - 96, tab_w = (body_w - 24) / 3;
    // Rebuild the invariant plates only when the physical canvas changes.
    struct Background {
        int width = 0, height = 0;
        bool wide = false;
        Picture pixels;
    };
    static Background background;
    if (background.width != width || background.height != height || background.wide != wide_canvas) {
        background.width = width;
        background.height = height;
        background.wide = wide_canvas;
        auto &pixels = background.pixels;
        pixels.width = wide_canvas ? width : pw;
        pixels.height = wide_canvas ? height : ph;
        pixels.x = wide_canvas ? 0 : px;
        pixels.y = wide_canvas ? 0 : py;
        pixels.rgba.assign(std::size_t(pixels.width) * pixels.height, premultiply(kScrim));
        Painter base(pixels, theme, scale);
        base.plate(Art::panel_main, box(0, 82, kW, 866));
        base.plate(Art::panel_header, box(100, 1, kW - 200, 152));
        base.plate(Art::crest_gold, box(63, 14, 146, 146));
        base.plate(Art::crest_gold, box(kW - 209, 14, 146, 146), true);
        base.plate(Art::divider_gold, box(196, 139, kW - 392, 13));
        base.plate(Art::panel_content, box(body_x, 217, body_w, 477));
        base.plate(Art::panel_description, box(body_x, 701, body_w, 75));
        base.plate(Art::divider_gold, box(45, 861, kW - 90, 8));
    }
    out = background.pixels;
    Painter paint(out, theme, scale, pw, s(110));
    if (!paint.valid()) return false;
    paint.text(box(204, 31, kW - 408, 71), 57, "Maximo Rex Settings", color(255, 226, 145), DT_CENTER, true, false,
               color(209, 132, 40));
    paint.text(box(202, 101, kW - 404, 37), 25, view_.profile, active_text, DT_CENTER, false);

    for (int i = 0; i < 3; ++i) {
        const int x = body_x + 12 + i * tab_w;
        const auto region = box(x, 154, tab_w, 67);
        tabs_[i] = hit(region);
        const bool is_active = i == static_cast<int>(tab_);
        paint.plate(is_active ? Art::tab_selected : Art::tab_normal, region);
        paint.text(box(x + 24, 163, tab_w - 48, 44), 34, kTabs[i], is_active ? ivory : muted, DT_CENTER);
    }

    constexpr int row_x = 65, row_y = 233, row_w = 1240, row_h = 55;
    const int n = static_cast<int>(view_.rows.size()), count = place_rows(8);
    for (int i = 0; i < count; ++i) {
        const int y = row_y + i * row_h;
        const auto region = box(row_x, y, row_w, row_h);
        visible_.push_back(hit(region));
        const auto &row = view_.rows[i + top_];
        const bool selected = selected_ == i + top_;
        if (selected) paint.plate(Art::selection_glow, box(row_x - 2, y - 7, row_w + 4, row_h + 14));
        paint.plate(selected ? Art::row_selected : Art::row_normal, region);
        const bool arrows = row.adjustable && row.enabled;
        const int value_x = row_x + 654, value_end = row_x + row_w - 25;
        paint.text(box(row_x + 32, y + 4, 604, row_h - 7), 28, row.label, row.enabled ? ivory : disabled);
        const auto value = box(value_x + (arrows ? 45 : 0), y + 4, value_end - value_x - (arrows ? 87 : 0), row_h - 7);
        int value_advance = value.w;
        paint.text(value, 27, row.value, row.enabled ? active_text : disabled, DT_RIGHT, true, false, 0,
                   arrows ? &value_advance : nullptr);
        Box left_arrow{};
        if (arrows) {
            left_arrow = box(value_x, y + 11, 33, 33);
            left_arrow.x = std::max(left_arrow.x, value.x + value.w - value_advance - s(44));
            paint.plate(Art::arrow_left, left_arrow);
            paint.plate(Art::arrow_right, box(value_end - 31, y + 11, 33, 33));
        }
        decrease_.push_back(arrows ? hit({left_arrow.x - s(6), region.y, s(45), region.h}) : Rect{});
    }
    if (count && count < n) {
        const int rail = count * row_h - 3, thumb = std::max(30, rail * count / n);
        const int offset = (rail - thumb) * top_ / (n - count);
        paint.plate(Art::scrollbar_track, box(1307, row_y, 23, rail));
        paint.plate(Art::scrollbar_thumb, box(1309, row_y + offset, 19, thumb));
    }

    std::string_view detail = view_.note;
    if (detail.empty() && selected_ < n) detail = view_.rows[selected_].detail;
    paint.text(box(body_x + 43, 707, body_w - 86, 61), 25, detail, view_.note.empty() ? ivory : active_text,
               DT_LEFT, false, true);

    if (view_.settings) {
        for (int i = 0; i < 3; ++i) {
            const int x = 106 + 406 * i;
            const auto region = box(x, 779, 368, 78);
            buttons_[i] = hit(region);
            const auto texture = i == pressed_button_ ? Art::button_pressed :
                i == hover_button_ || selected_ == n + i || i == 2 ? Art::button_hover : Art::button_normal;
            paint.plate(texture, region);
            paint.text(box(x + 50, 784 + (i == pressed_button_ ? 2 : 0), 268, 60), 32, kButtons[i], ivory, DT_CENTER);
            if (selected_ == n + i) paint.plate(Art::divider_gold, box(x + 122, 838, 124, 6));
        }
    }

    const auto keycap = [&](int x, int w, std::string_view label) {
        paint.fill(box(x, 882, w, 32), brass);
        paint.fill(box(x + 1, 883, w - 2, 30), color(32, 23, 20));
        paint.text(box(x + 1, 882, w - 2, 31), 19, label, ivory, DT_CENTER);
    };
    keycap(135, 57, "Tab");
    keycap(211, 44, "LB");
    keycap(263, 44, "RB");
    paint.text(box(316, 879, 102, 38), 22, "abas", muted);
    keycap(530, 146, "Ctrl+Enter");
    keycap(687, 35, "Y");
    paint.text(box(735, 879, 108, 38), 22, "aplicar", muted);
    keycap(1004, 49, "F1");
    paint.text(box(1066, 879, 198, 38), 22, "fechar", muted);
    paint.finish();
    return true;
}

}  // namespace quickmenu

#else

bool quickmenu::Menu::draw_medieval(int, int, Picture &, bool) { return false; }

#endif
