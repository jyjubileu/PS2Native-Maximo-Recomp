#pragma once

#include <cstdint>
#include <webgpu/webgpu.h>

namespace ps2::gs::gpu::aa {

enum class Mode { off, fxaa, smaa };

// Intermediates cover only the game's viewport, excluding the bars and host overlay.
bool prepare(WGPUDevice device, WGPUQueue queue, WGPUTextureFormat output,
             std::uint32_t width, std::uint32_t height, Mode mode, unsigned cas_strength);
WGPUTextureView scene();
void process(WGPUCommandEncoder encoder);
void draw(WGPURenderPassEncoder pass);

}  // namespace ps2::gs::gpu::aa
