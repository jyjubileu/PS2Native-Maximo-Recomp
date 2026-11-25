#include "maximo_weapons.hpp"

#include "ps2/generated.hpp"
#include "ps2/memory.hpp"
#include "ps2/system.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>

namespace maximo {
namespace {

// SLES_517.59 PAL. Player/profile pointers and rows are read by the original
// inventory.
constexpr std::uint32_t kEquipmentPlayer = 0x0032bfac;
constexpr std::uint32_t kInventoryProfile = 0x00307fc8;
constexpr std::uint32_t kMode = 0x00345490, kNextMode = kMode + 4;
constexpr std::uint32_t kInventoryPhase = 0x00345734;
constexpr std::uint32_t kEquipped = 0x1fc, kAction = 0x260, kPlayerType = 0x229;
constexpr std::array<std::uint32_t, 4> kSlots{0x22b, 0x22c, 0x23b, 0x23c};

constexpr std::uint32_t kEquip = 0x0015c860;
constexpr std::uint32_t kResume = 0x00165f30;
constexpr std::uint32_t kPlayerUpdateCaller = 0x00148150;
constexpr std::uint32_t kCallerStackOffset = 32;
constexpr std::uint32_t kModelTable = 0x002568b0;
constexpr std::uint32_t kModel = 0x284, kModelAsset = 0x10;
constexpr std::uint32_t kIdle = 0, kWalk = 1, kRun = 2;
constexpr std::uint64_t kEquipWaitFrames = 12;

bool locomotion(std::uint32_t action) {
    return action == kIdle || action == kWalk || action == kRun;
}

bool pal_equipment() {
#ifdef PS2_MAXIMO_STYLE_SWITCH
    return ps2::ld32(kResume - 8) == 0x0c0586cc && ps2::ld32(kResume) == 0x0200202d &&
           ps2::ld32(kResume + 4) == 0x0c0594b0 && ps2::ld32(kPlayerUpdateCaller - 8) == 0x0320f809 &&
           ps2::ld32(0x00165ca0) == 0x27bdffb0 && ps2::ld32(kEquip) == 0x27bdffe0 &&
           ps2::ld32(0x0015c8c8) == 0xae0501fc && ps2::ld32(0x00161c60) == 0x27bdffb0 &&
           ps2::ld32(0x00161480) == 0x27bdff60;
#else
    return false;
#endif
}

bool ram_pointer(std::uint32_t address, std::uint32_t bytes) {
    return address >= 0x00100000 && address <= ps2::kRamSize - bytes && (address & 3u) == 0;
}

// Read the resident class-1 model table used by 0x156210. The four weapon keys
// select its global registry (gp-0x7c24). It may have a local-ID map.
std::uint32_t resident_weapon_mesh(std::uint32_t gp, int weapon) {
    const auto key = ps2::ld32(kModelTable + 16 * weapon);
    if ((key >> 24) != 0x81 || !ram_pointer(gp - 0x7c24, 4)) return 0;
    const auto group = ps2::ld32(gp - 0x7c24);
    if (!ram_pointer(group, 12)) return 0;
    const auto count = ps2::ld32(group);
    if (count == 0 || count > 0x10000) return 0;
    const auto entries = ps2::ld32(group + 4);
    if (!ram_pointer(entries, count * 4)) return 0;

    const auto asset_id = key & 0x00ffffff;
    auto index = asset_id;
    const auto map = ps2::ld32(group + 8);
    if (map) {
        if (!ram_pointer(map, count * 4)) return 0;
        index = 0;
        while (index < count && ps2::ld32(map + index * 4) != asset_id)
            ++index;
    }
    if (index >= count) return 0;
    const auto asset = ps2::ld32(entries + index * 4);
    if (!ram_pointer(asset, 0x14)) return 0;
    const auto mesh = ps2::ld32(asset + kModelAsset);
    return ram_pointer(mesh, 0x10) ? mesh : 0;
}

bool can_equip_weapon(std::uint32_t player, std::uint32_t action, int equipped, int target) {
    if (!locomotion(action)) return false;
    // Match the ready-state tests in the gameplay handler at 0x1652c0.
    const auto flags = ps2::ld32(player + 0x264);
    if (!(flags & 0x30) || (flags & 4) || ps2::ld8(player + kPlayerType) >= 4 ||
        ps2::ld8(player + 0x339) != 0xff) return false;
    const auto player_flags = ps2::ld32(player + 0x1d0);
    if (!(player_flags & 4)) return false;
    // The original L2 path at 0x1676b8 requires the rig to allow a family change.
    if ((equipped ^ target) & 2) {
        const auto rig = ps2::ld32(player + 0x38);
        if (!ram_pointer(rig, 268) || !(ps2::ld32(rig + 264) & 1u)) return false;
    }

    const auto model = ps2::ld32(player + kModel);
    const auto properties = ps2::ld32(player + 0x3c);
    const auto animation_bank = ps2::ld32(player + 0x25c);
    if (!ram_pointer(model, 216) || !ram_pointer(properties, 304) ||
        !ram_pointer(animation_bank, 6) || action >= ps2::ld16(animation_bank + 4)) return false;
    const auto animation_table = ps2::ld32(animation_bank);
    if (!ram_pointer(animation_table, (action + 1) * 28)) return false;
    const auto animation = animation_table + action * 28;
    return ps2::ld32(animation) && ((player_flags & 1) || (ps2::ld32(animation + 8) & 8));
}

}  // namespace

bool WeaponWheel::State::gameplay() const {
    return player && mode == 0 && next_mode == 0 && menu_step == 0xffffffffu;
}

bool WeaponWheel::State::can_open_wheel() const {
    return gameplay() && player_type < 4 && locomotion(action);
}

WeaponWheel::State WeaponWheel::inspect() {
    State state;
    state.supported = pal_equipment();
    if (!state.supported) return state;
    const auto player = ps2::ld32(kEquipmentPlayer);
    if (player < 0x00500000 || !ram_pointer(player, 0x340) ||
        ps2::ld32(kInventoryProfile) != player + 0x1d0) return state;

    const auto equipped = ps2::ld32(player + kEquipped);
    if (equipped >= 4) return state;
    for (std::size_t i = 0; i < kSlots.size(); ++i) {
        const auto weapon = ps2::ld8(player + kSlots[i]);
        if (weapon == 0xff) continue;
        if (weapon >= 4 || weapon / 2u != i / 2 || (state.available & (1u << weapon))) return state;
        state.available |= 1u << weapon;
    }
    if (!(state.available & (1u << equipped))) return state;

    state.player = player;
    state.equipped = static_cast<int>(equipped);
    state.mode = ps2::ld32(kMode);
    state.next_mode = ps2::ld32(kNextMode);
    state.menu_step = ps2::ld32(kInventoryPhase);
    state.action = ps2::ld32(player + kAction);
    state.player_type = ps2::ld8(player + kPlayerType);
    return state;
}

bool WeaponWheel::supported() const {
    std::lock_guard lock(mutex_);
    return state_.supported;
}

bool WeaponWheel::has_recent_player_update() const {
    return state_.player == update_player_ && previous_frame_ >= update_frame_ &&
           previous_frame_ - update_frame_ <= 2;
}

bool WeaponWheel::open() {
    std::lock_guard lock(mutex_);
    const bool updating = has_recent_player_update();
    if (phase_ != Phase::idle || !state_.can_open_wheel() || !updating) {
        if (state_.supported && phase_ == Phase::idle)
            std::fprintf(stderr, "[wheel] blocked f=%llu mode=%u step=%08x action=%u update=%d\n",
                         static_cast<unsigned long long>(previous_frame_), state_.mode, state_.menu_step, state_.action, updating);
        return false;
    }
    phase_ = Phase::wheel;
    selected_ = state_.equipped;
    player_ = state_.player;
    std::fprintf(stderr, "[wheel] open f=%llu equipped=%d available=%x action=%u\n",
                 static_cast<unsigned long long>(previous_frame_), state_.equipped, state_.available, state_.action);
    return true;
}

void WeaponWheel::select(int weapon) {
    std::lock_guard lock(mutex_);
    if (phase_ != Phase::wheel || weapon < 0 || weapon >= 4 || weapon == selected_) return;
    selected_ = weapon;
    std::fprintf(stderr, "[wheel] %s f=%llu weapon=%d\n", (state_.available & (1u << weapon)) ? "highlighted" : "locked",
                 static_cast<unsigned long long>(previous_frame_), weapon);
}

void WeaponWheel::stick(int x, int y) {
    if (std::max(std::abs(x), std::abs(y)) < 13000) return;
    select(std::abs(x) > std::abs(y) ? (x > 0 ? 1 : 2) : (y > 0 ? 3 : 0));
}

void WeaponWheel::release() {
    std::lock_guard lock(mutex_);
    if (phase_ != Phase::wheel) return;
    if (!state_.gameplay() || state_.player != player_ || !(state_.available & (1u << selected_))) {
        finish("unavailable");
    } else if (selected_ == state_.equipped) {
        finish("already-equipped");
    } else {
        queue_weapon(selected_);
    }
}

bool WeaponWheel::can_capture_dpad() const {
    std::lock_guard lock(mutex_);
    return state_.can_open_wheel() && has_recent_player_update();
}

void WeaponWheel::request_weapon(int weapon) {
    std::lock_guard lock(mutex_);
    if (weapon < 0 || weapon >= 4 || !state_.can_open_wheel() || !has_recent_player_update()) return;
    if (phase_ != Phase::idle && (phase_ != Phase::pending || player_ != state_.player)) return;
    if (!(state_.available & (1u << weapon))) {
        std::fprintf(stderr, "[dpad] locked f=%llu weapon=%d\n", static_cast<unsigned long long>(previous_frame_), weapon);
        return;
    }
    if (state_.equipped == weapon) {
        if (phase_ == Phase::pending) finish("already-equipped");
        return;
    }
    // A new direction can replace the outstanding request for this player.
    queue_weapon(weapon);
}

void WeaponWheel::queue_weapon(int weapon) {
    player_ = state_.player;
    target_ = weapon;
    phase_ = Phase::pending;
    deadline_ = previous_frame_ + kEquipWaitFrames;
    waiting_for_safe_action_ = false;
    std::fprintf(stderr, "[wheel] queued f=%llu target=%d action=%u deadline=%llu\n",
                 static_cast<unsigned long long>(previous_frame_), target_, state_.action,
                 static_cast<unsigned long long>(deadline_));
}

void WeaponWheel::cancel() {
    std::lock_guard lock(mutex_);
    if (phase_ == Phase::wheel || phase_ == Phase::pending) finish("cancelled");
}

bool WeaponWheel::can_cancel() const {
    std::lock_guard lock(mutex_);
    return phase_ == Phase::wheel || phase_ == Phase::pending;
}

WheelView WeaponWheel::view() const {
    std::lock_guard lock(mutex_);
    if (phase_ != Phase::wheel) return {};
    return {true, selected_, state_.equipped, state_.available};
}

void WeaponWheel::finish(const char *outcome) {
    std::fprintf(stderr, "[wheel] %s f=%llu target=%d equipped=%d\n", outcome,
                 static_cast<unsigned long long>(previous_frame_), target_, state_.equipped);
    phase_ = Phase::idle;
    selected_ = target_ = -1;
}

void WeaponWheel::check_return(const ps2::Cpu &cpu) {
    if (phase_ != Phase::switching || cpu.r[29].w[0] != return_stack_ ||
        cpu.r[16].w[0] != player_ || cpu.r[31].w[0] != kResume) return;
    state_ = inspect();
    const auto model = ps2::ld32(player_ + kModel);
    const auto properties = ps2::ld32(player_ + 0x3c);
    const auto mesh = ram_pointer(model, 0x14) ? ps2::ld32(model + kModelAsset) : 0;
    const auto property = ram_pointer(properties, 304) ? ps2::ld32(properties + 300) : 0;
    const auto expected_property = ps2::ld32(kModelTable + 16 * target_ + 12);
    std::fprintf(stderr, "[wheel] return f=%llu weapon=%d mode=%u next=%u inventory=%08x model=%08x mesh=%08x expected=%08x property=%08x action=%u\n",
                 static_cast<unsigned long long>(ps2::frame_count()), state_.equipped, state_.mode, state_.next_mode, state_.menu_step,
                 model, mesh, expected_mesh_, property, ps2::ld32(player_ + kAction));
    finish(state_.can_open_wheel() && state_.player == player_ && state_.equipped == target_ &&
           mesh == expected_mesh_ && property == expected_property ? "equipped" : "unexpected-result");
}

bool WeaponWheel::equipment_point(ps2::Cpu &cpu, std::uint32_t depth) {
    const auto player = cpu.r[16].w[0];
    const auto sp = cpu.r[29].w[0];
    const auto frame = ps2::frame_count();
    if (!player || player != ps2::ld32(kEquipmentPlayer) || cpu.r[31].w[0] != kResume ||
        !ram_pointer(sp, kCallerStackOffset + 4) || (sp & 15u) ||
        ps2::ld32(sp + kCallerStackOffset) != kPlayerUpdateCaller) return false;

    {
        std::lock_guard lock(mutex_);
        update_player_ = player;
        update_frame_ = frame;
        check_return(cpu);
        if (phase_ != Phase::pending) return false;

        state_ = inspect();
        if (frame < previous_frame_ || frame >= deadline_ || !state_.gameplay() || player != player_ ||
            state_.player != player || !(state_.available & (1u << target_))) {
            finish("context-changed");
            return false;
        }
        if (!can_equip_weapon(player, state_.action, state_.equipped, target_)) {
            if (!waiting_for_safe_action_) {
                std::fprintf(stderr, "[wheel] wait f=%llu target=%d action=%u\n",
                             static_cast<unsigned long long>(frame), target_, state_.action);
                waiting_for_safe_action_ = true;
            }
            return false;
        }
        if (state_.equipped == target_) {
            finish("already-equipped");
            return false;
        }
        expected_mesh_ = resident_weapon_mesh(cpu.r[28].w[0], target_);
        if (!expected_mesh_) {
            finish("model-not-resident");
            return false;
        }

        return_stack_ = sp;
        phase_ = Phase::switching;
        deadline_ = frame + 4;
        std::fprintf(stderr, "[wheel] call f=%llu from=%d to=%d player=%08x sp=%08x gp=%08x ra=%08x parent=%08x mesh=%08x action=%u\n",
                     static_cast<unsigned long long>(frame), state_.equipped, target_, player_, sp, cpu.r[28].w[0], kResume,
                     kPlayerUpdateCaller, expected_mesh_, state_.action);
        // The preceding guest call already returned here with this RA. The
        // next instruction rebuilds a0 from s0 and the next call takes only a0.
        cpu.r[4].d[0] = cpu.r[16].d[0];
        cpu.r[5].d[0] = static_cast<std::uint32_t>(target_);
    }

    // An EE yield keeps the real PC and stack, exactly as with a generated jal.
    ps2::call(cpu, kEquip, depth + 1);
    if (cpu.pc != kResume) return true;
    std::lock_guard lock(mutex_);
    check_return(cpu);
    return false;
}

void WeaponWheel::frame() {
    if (!ps2::g_options.replay_path.empty()) return;
    const auto state = inspect();
    const auto frame = ps2::frame_count();
    std::lock_guard lock(mutex_);
    state_ = state;
    const bool discontinuity = previous_frame_ && frame != previous_frame_ + 1;
    if (discontinuity) update_player_ = 0;
    previous_frame_ = frame;
    if (phase_ == Phase::idle) return;
    if (discontinuity) finish("state-loaded");
    else if (phase_ != Phase::switching && (!state_.gameplay() || state_.player != player_))
        finish("context-changed");
    else if (phase_ == Phase::pending && frame >= deadline_) finish("equip-timeout");
    else if (phase_ == Phase::switching && frame >= deadline_) finish("ee-return-not-seen");
}

}  // namespace maximo
