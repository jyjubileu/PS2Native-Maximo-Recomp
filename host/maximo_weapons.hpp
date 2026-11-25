#pragma once

#include <cstdint>
#include <mutex>

namespace ps2 {
struct Cpu;
}

namespace maximo {

struct WheelView {
    bool open = false;
    int selected = -1;
    int equipped = -1;
    unsigned available = 0;
    bool operator==(const WheelView &) const = default;
};

// The window owns physical input. Equipment requests are consumed only at the
// PAL game's recompiled player update, on the emulation thread.
class WeaponWheel {
public:
    bool supported() const;
    bool open();
    void select(int weapon);
    void stick(int x, int y);
    void release();
    bool can_capture_dpad() const;
    void request_weapon(int weapon);
    void cancel();
    bool can_cancel() const;
    WheelView view() const;
    void frame();
    bool equipment_point(ps2::Cpu &cpu, std::uint32_t depth);

private:
    struct State {
        bool supported = false;
        std::uint32_t player = 0;
        std::uint32_t mode = 0, next_mode = 0;
        std::uint32_t menu_step = 0;
        std::uint32_t action = 0xffffffffu;
        std::uint8_t player_type = 0xff;
        int equipped = -1;
        unsigned available = 0;
        bool gameplay() const;
        bool can_open_wheel() const;
    };
    enum class Phase { idle, wheel, pending, switching };

    static State inspect();
    bool has_recent_player_update() const;
    void queue_weapon(int weapon);
    void finish(const char *outcome);
    void check_return(const ps2::Cpu &cpu);

    mutable std::mutex mutex_;
    State state_{};
    Phase phase_ = Phase::idle;
    int selected_ = -1;
    int target_ = -1;
    std::uint32_t player_ = 0;
    std::uint32_t update_player_ = 0;
    std::uint32_t return_stack_ = 0;
    std::uint32_t expected_mesh_ = 0;
    bool waiting_for_safe_action_ = false;
    std::uint64_t previous_frame_ = 0, update_frame_ = 0, deadline_ = 0;
};

// True means the EE yielded inside the original routine. Use its real PC
// so the dispatcher can resume it and eventually return to this same site.
bool weapon_equipment_point(ps2::Cpu &cpu, std::uint32_t depth);

}  // namespace maximo
