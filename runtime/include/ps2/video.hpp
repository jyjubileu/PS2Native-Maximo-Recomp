#pragma once
#include <cstdint>

namespace ps2 {
// Display-only options, independent of emulated GS state and save states.
// bit 0: edge antialiasing.
// bit 2: sharp enlargement (cubic plus light contrast-adaptive sharpening), the other two bits are not used with it.
// bit 1: soft enlargement (a smooth blend everywhere) instead of the sharp one (hard pixels, only their borders blend).
// bit 3: present the current interlaced frame field without averaging the previous field.
inline constexpr std::uint32_t kVideoCurrentField = 1u << 3;
// bit 4: align opposite frame-mode fields in the WebGPU compositor before averaging.
inline constexpr std::uint32_t kVideoAlignedFields = 1u << 4;
// bit 5: use previous fields only where same-parity history indicates little motion.
inline constexpr std::uint32_t kVideoMotionFields = 1u << 5;
// bit 6: integrate the exact source footprint when reducing the WebGPU picture.
inline constexpr std::uint32_t kVideoAreaReduction = 1u << 6;
// bit 7: check motion in both parities before blending aligned fields.
inline constexpr std::uint32_t kVideoDualMotionFields = 1u << 7;
// At most one output-resolution GPU filter is selected.
inline constexpr std::uint32_t kVideoFXAA = 1u << 8;
inline constexpr std::uint32_t kVideoSMAA = 1u << 9;
// Bits 10–16 hold the effective CAS strength from 0 to 100 percent.
inline constexpr unsigned kVideoCasShift = 10;
inline constexpr std::uint32_t kVideoCasMask = 127u << kVideoCasShift;
void set_video_options(std::uint32_t options);
std::uint32_t video_options();
}
