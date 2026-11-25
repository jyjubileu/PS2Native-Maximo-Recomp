#include "postaa.hpp"

#include <cstdio>
#include <cstring>
#include <initializer_list>

extern const char kPostAAShader[];
extern const unsigned char kSmaaArea[];
extern const unsigned char kSmaaSearch[];

namespace ps2::gs::gpu::aa {
namespace {

WGPUStringView view(const char *label) { return {label, std::strlen(label)}; }

struct Image {
    WGPUTexture texture = nullptr;
    WGPUTextureView view = nullptr;

    void release() {
        if (view) wgpuTextureViewRelease(view);
        if (texture) wgpuTextureRelease(texture);
        view = nullptr;
        texture = nullptr;
    }
};

Image g_scene, g_edges, g_weights, g_area, g_search, g_aa_result;
WGPUTextureView g_scene_linear = nullptr;
WGPUTextureView g_aa_result_linear = nullptr;
WGPUSampler g_bilinear = nullptr;
WGPUShaderModule g_shader = nullptr;
WGPURenderPipeline g_fxaa[2]{}, g_smaa[2]{};
WGPURenderPipeline g_find_edges = nullptr, g_find_weights = nullptr, g_aa = nullptr, g_cas = nullptr;
WGPUBindGroup g_edge_inputs = nullptr, g_weight_inputs = nullptr, g_aa_inputs = nullptr, g_cas_inputs = nullptr;
WGPUBuffer g_cas_params = nullptr;
Mode g_mode = Mode::off;
std::uint32_t g_width = 0, g_height = 0;
unsigned g_cas_strength = 0;
bool g_cas_active = false, g_ready = false;
constexpr auto kTargetUsage = WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_TextureBinding;

Image make_image(WGPUDevice device, WGPUTextureFormat format, WGPUTextureUsage usage,
                 std::uint32_t width, std::uint32_t height, const char *label, bool srgb_view = false) {
    WGPUTextureDescriptor d = WGPU_TEXTURE_DESCRIPTOR_INIT;
    d.label = view(label);
    d.dimension = WGPUTextureDimension_2D;
    d.size = {width, height, 1};
    d.format = format;
    d.usage = usage;
    const WGPUTextureFormat srgb = WGPUTextureFormat_RGBA8UnormSrgb;
    if (srgb_view) {
        d.viewFormatCount = 1;
        d.viewFormats = &srgb;
    }
    const auto texture = wgpuDeviceCreateTexture(device, &d);
    return {texture, texture ? wgpuTextureCreateView(texture, nullptr) : nullptr};
}

Image upload_table(WGPUDevice device, WGPUQueue queue, WGPUTextureFormat format, const unsigned char *data,
                   std::uint32_t width, std::uint32_t height, unsigned channels, const char *label) {
    auto image = make_image(device, format, WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst,
                            width, height, label);
    if (!image.texture) return image;
    WGPUTexelCopyTextureInfo dst = WGPU_TEXEL_COPY_TEXTURE_INFO_INIT;
    dst.texture = image.texture;
    WGPUTexelCopyBufferLayout layout = WGPU_TEXEL_COPY_BUFFER_LAYOUT_INIT;
    layout.bytesPerRow = width * channels;
    layout.rowsPerImage = height;
    const WGPUExtent3D size{width, height, 1};
    wgpuQueueWriteTexture(queue, &dst, data, std::size_t(width) * height * channels, &layout, &size);
    return image;
}

WGPURenderPipeline pipeline(WGPUDevice device, WGPUTextureFormat format, const char *entry) {
    WGPUColorTargetState target = WGPU_COLOR_TARGET_STATE_INIT;
    target.format = format;
    WGPUFragmentState fragment = WGPU_FRAGMENT_STATE_INIT;
    fragment.module = g_shader;
    fragment.entryPoint = view(entry);
    fragment.targetCount = 1;
    fragment.targets = &target;
    WGPURenderPipelineDescriptor desc = WGPU_RENDER_PIPELINE_DESCRIPTOR_INIT;
    desc.label = view(entry);
    desc.vertex.module = g_shader;
    desc.vertex.entryPoint = view("vs");
    desc.fragment = &fragment;
    return wgpuDeviceCreateRenderPipeline(device, &desc);
}

WGPUBindGroupEntry texture(std::uint32_t binding, WGPUTextureView image) {
    WGPUBindGroupEntry e = WGPU_BIND_GROUP_ENTRY_INIT;
    e.binding = binding;
    e.textureView = image;
    return e;
}

WGPUBindGroup bind(WGPUDevice device, WGPURenderPipeline pass, std::initializer_list<WGPUBindGroupEntry> inputs) {
    if (!pass) return nullptr;
    WGPUBindGroupDescriptor d = WGPU_BIND_GROUP_DESCRIPTOR_INIT;
    d.layout = wgpuRenderPipelineGetBindGroupLayout(pass, 0);
    d.entryCount = inputs.size();
    d.entries = inputs.begin();
    const auto group = wgpuDeviceCreateBindGroup(device, &d);
    wgpuBindGroupLayoutRelease(d.layout);
    return group;
}

void reset_images() {
    for (auto *group : {&g_edge_inputs, &g_weight_inputs, &g_aa_inputs, &g_cas_inputs}) {
        if (*group) wgpuBindGroupRelease(*group);
        *group = nullptr;
    }
    for (auto *linear : {&g_scene_linear, &g_aa_result_linear}) {
        if (*linear) wgpuTextureViewRelease(*linear);
        *linear = nullptr;
    }
    g_edges.release();
    g_weights.release();
    g_scene.release();
    g_aa_result.release();
}

WGPUTextureView linear_view(WGPUTexture texture) {
    WGPUTextureViewDescriptor desc = WGPU_TEXTURE_VIEW_DESCRIPTOR_INIT;
    desc.format = WGPUTextureFormat_RGBA8UnormSrgb;
    return texture ? wgpuTextureCreateView(texture, &desc) : nullptr;
}

bool build_aa(WGPUDevice device, WGPUQueue queue, WGPUTextureFormat output) {
    if (!g_bilinear) {
        WGPUSamplerDescriptor d = WGPU_SAMPLER_DESCRIPTOR_INIT;
        d.minFilter = d.magFilter = WGPUFilterMode_Linear;
        g_bilinear = wgpuDeviceCreateSampler(device, &d);
    }
    WGPUBindGroupEntry sampler = WGPU_BIND_GROUP_ENTRY_INIT;
    sampler.sampler = g_bilinear;
    // AA writes directly to the surface, or to CAS's RGBA8 input when CAS is enabled.
    const auto target = g_cas_active ? WGPUTextureFormat_RGBA8Unorm : output;
    if (g_mode == Mode::fxaa) {
        auto &fxaa = g_fxaa[g_cas_active];
        if (!fxaa) fxaa = pipeline(device, target, "fxaa");
        g_aa = fxaa;
        g_aa_inputs = bind(device, g_aa, {sampler, texture(1, g_scene.view)});
        return g_aa_inputs != nullptr;
    }

    if (!g_find_edges) g_find_edges = pipeline(device, WGPUTextureFormat_RG8Unorm, "smaa_edges");
    if (!g_find_weights) g_find_weights = pipeline(device, WGPUTextureFormat_RGBA8Unorm, "smaa_weights");
    auto &smaa = g_smaa[g_cas_active];
    if (!smaa) smaa = pipeline(device, target, "smaa_neighborhood");
    g_aa = smaa;
    if (!g_area.view) g_area = upload_table(device, queue, WGPUTextureFormat_RG8Unorm, kSmaaArea, 160, 80, 2, "SMAA area");
    if (!g_search.view) g_search = upload_table(device, queue, WGPUTextureFormat_R8Unorm, kSmaaSearch, 64, 16, 1, "SMAA search");
    g_edges = make_image(device, WGPUTextureFormat_RG8Unorm, kTargetUsage, g_width, g_height, "SMAA edges");
    g_weights = make_image(device, WGPUTextureFormat_RGBA8Unorm, kTargetUsage, g_width, g_height, "SMAA weights");
    g_edge_inputs = bind(device, g_find_edges, {texture(1, g_scene.view)});
    g_weight_inputs = bind(device, g_find_weights, {sampler, texture(2, g_edges.view), texture(3, g_area.view), texture(4, g_search.view)});
    g_aa_inputs = bind(device, g_aa, {sampler, texture(1, g_scene.view), texture(5, g_weights.view), texture(6, g_scene_linear)});
    return g_edge_inputs && g_weight_inputs && g_aa_inputs;
}

bool build(WGPUDevice device, WGPUQueue queue, WGPUTextureFormat output) {
    if (!g_shader) {
        WGPUShaderSourceWGSL source = WGPU_SHADER_SOURCE_WGSL_INIT;
        source.code = view(kPostAAShader);
        WGPUShaderModuleDescriptor md = WGPU_SHADER_MODULE_DESCRIPTOR_INIT;
        md.nextInChain = &source.chain;
        md.label = view("output AA and CAS");
        g_shader = wgpuDeviceCreateShaderModule(device, &md);
    }
    const bool reads_scene_linear = g_mode == Mode::smaa || (g_mode == Mode::off && g_cas_active);
    g_scene = make_image(device, WGPUTextureFormat_RGBA8Unorm, kTargetUsage, g_width, g_height, "Presentation input", reads_scene_linear);
    if (!g_scene.view) return false;
    if (reads_scene_linear) {
        g_scene_linear = linear_view(g_scene.texture);
        if (!g_scene_linear) return false;
    }
    if (g_mode != Mode::off && !build_aa(device, queue, output)) return false;
    if (!g_cas_active) return true;

    auto input = g_scene_linear;
    if (g_mode != Mode::off) {
        g_aa_result = make_image(device, WGPUTextureFormat_RGBA8Unorm, kTargetUsage, g_width, g_height, "CAS input after AA", true);
        g_aa_result_linear = linear_view(g_aa_result.texture);
        input = g_aa_result_linear;
        if (!g_aa_result.view || !input) return false;
    }
    if (!g_cas) g_cas = pipeline(device, output, "cas_sharpen");
    if (!g_cas_params) {
        WGPUBufferDescriptor desc = WGPU_BUFFER_DESCRIPTOR_INIT;
        desc.label = view("CAS constants");
        desc.size = 16;
        desc.usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst;
        g_cas_params = wgpuDeviceCreateBuffer(device, &desc);
    }
    if (!g_cas_params) return false;
    WGPUBindGroupEntry constants = WGPU_BIND_GROUP_ENTRY_INIT;
    constants.binding = 8;
    constants.buffer = g_cas_params;
    constants.size = 16;
    g_cas_inputs = bind(device, g_cas, {texture(7, input), constants});
    return g_cas_inputs != nullptr;
}

void set_cas_strength(WGPUQueue queue, unsigned percent) {
    if (g_cas_strength == percent) return;
    g_cas_strength = percent;
    // CasSetup const1[0]. At 1:1, CasFilter does not use the scaling constants.
    const float strength = static_cast<float>(percent) / 100.0f;
    const float constants[4] = {-1.0f / (8.0f - 3.0f * strength), 0.0f, 0.0f, 0.0f};
    wgpuQueueWriteBuffer(queue, g_cas_params, 0, constants, sizeof constants);
}

void fullscreen(WGPURenderPassEncoder pass, WGPURenderPipeline pipeline, WGPUBindGroup inputs) {
    wgpuRenderPassEncoderSetPipeline(pass, pipeline);
    wgpuRenderPassEncoderSetBindGroup(pass, 0, inputs, 0, nullptr);
    wgpuRenderPassEncoderDraw(pass, 3, 1, 0, 0);
}

void render(WGPUCommandEncoder encoder, Image output, WGPURenderPipeline pipeline, WGPUBindGroup inputs, const char *label) {
    WGPURenderPassColorAttachment color = WGPU_RENDER_PASS_COLOR_ATTACHMENT_INIT;
    color.view = output.view;
    color.loadOp = WGPULoadOp_Clear;
    color.storeOp = WGPUStoreOp_Store;
    WGPURenderPassDescriptor desc = WGPU_RENDER_PASS_DESCRIPTOR_INIT;
    desc.label = view(label);
    desc.colorAttachmentCount = 1;
    desc.colorAttachments = &color;
    const auto pass = wgpuCommandEncoderBeginRenderPass(encoder, &desc);
    fullscreen(pass, pipeline, inputs);
    wgpuRenderPassEncoderEnd(pass);
    wgpuRenderPassEncoderRelease(pass);
}

}  // namespace

bool prepare(WGPUDevice device, WGPUQueue queue, WGPUTextureFormat output,
             std::uint32_t width, std::uint32_t height, Mode mode, unsigned cas_strength) {
    if (!width || !height) {
        mode = Mode::off;
        cas_strength = 0;
    }
    const bool cas = cas_strength > 0;
    const bool filtering = mode != Mode::off || cas;
    if (mode != g_mode || cas != g_cas_active || (filtering && (width != g_width || height != g_height))) {
        reset_images();
        g_mode = mode;
        g_cas_active = cas;
        g_width = width;
        g_height = height;
        g_ready = filtering && build(device, queue, output);
        if (filtering && !g_ready) {
            reset_images();
            std::fprintf(stderr, "[gpu] output AA/CAS unavailable; using the original presentation\n");
        }
    }
    if (g_ready && cas) set_cas_strength(queue, cas_strength);
    return g_ready;
}

WGPUTextureView scene() { return g_scene.view; }

void process(WGPUCommandEncoder encoder) {
    if (g_mode == Mode::smaa) {
        render(encoder, g_edges, g_find_edges, g_edge_inputs, "SMAA edges");
        render(encoder, g_weights, g_find_weights, g_weight_inputs, "SMAA weights");
    }
    if (g_cas_active && g_mode != Mode::off)
        render(encoder, g_aa_result, g_aa, g_aa_inputs, "AA before CAS");
}

void draw(WGPURenderPassEncoder pass) {
    if (g_cas_active) fullscreen(pass, g_cas, g_cas_inputs);
    else fullscreen(pass, g_aa, g_aa_inputs);
}

}  // namespace ps2::gs::gpu::aa
