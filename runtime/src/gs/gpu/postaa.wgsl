// Output-pixel anti-aliasing. See smaa/LICENSE.txt for the SMAA 1x port and lookup textures.
@group(0) @binding(0) var bilinear: sampler;
@group(0) @binding(1) var scene_color: texture_2d<f32>;
@group(0) @binding(2) var edges_tex: texture_2d<f32>;
@group(0) @binding(3) var area_tex: texture_2d<f32>;
@group(0) @binding(4) var search_tex: texture_2d<f32>;
@group(0) @binding(5) var weights_tex: texture_2d<f32>;
@group(0) @binding(6) var scene_linear: texture_2d<f32>;
@group(0) @binding(7) var cas_source_linear: texture_2d<f32>;
@group(0) @binding(8) var<uniform> cas_constants: vec4<f32>;

struct Vertex {
    @builtin(position) position: vec4<f32>,
    @location(0) uv: vec2<f32>,
}

@vertex
fn vs(@builtin(vertex_index) i: u32) -> Vertex {
    let p = vec2<f32>(f32((i << 1u) & 2u), f32(i & 2u));
    return Vertex(vec4<f32>(p * 2.0 - 1.0, 0.0, 1.0), vec2<f32>(p.x, 1.0 - p.y));
}

fn clamp_pixel(p: vec2<i32>, size: vec2<u32>) -> vec2<i32> {
    return clamp(p, vec2<i32>(0), vec2<i32>(size) - 1);
}

fn read_color(p: vec2<i32>) -> vec3<f32> {
    return textureLoad(scene_color, clamp_pixel(p, textureDimensions(scene_color)), 0).rgb;
}

fn luma(rgb: vec3<f32>) -> f32 {
    return dot(rgb, vec3<f32>(0.299, 0.587, 0.114));
}

// Lottes's quality FXAA: measure contrast, find the span of the dominant edge,
// then combine the endpoint and subpixel offsets in one color lookup.
@fragment
fn fxaa(in: Vertex) -> @location(0) vec4<f32> {
    let dimensions = textureDimensions(scene_color);
    let p = clamp_pixel(vec2<i32>(in.uv * vec2<f32>(dimensions)), dimensions);
    let color = read_color(p);
    let m = luma(color);
    let n = luma(read_color(p + vec2<i32>(0, -1)));
    let s = luma(read_color(p + vec2<i32>(0,  1)));
    let w = luma(read_color(p + vec2<i32>(-1, 0)));
    let e = luma(read_color(p + vec2<i32>( 1, 0)));
    let high = max(m, max(max(n,s),max(w,e)));
    let range = high - min(m, min(min(n,s),min(w,e)));
    if (range < max(0.0625, 0.166 * high)) { return vec4<f32>(color, 1.0); }

    let nw = luma(read_color(p + vec2<i32>(-1, -1)));
    let ne = luma(read_color(p + vec2<i32>( 1, -1)));
    let sw = luma(read_color(p + vec2<i32>(-1,  1)));
    let se = luma(read_color(p + vec2<i32>( 1,  1)));
    let horizontal = abs(nw + sw - 2.0*w) + 2.0*abs(n + s - 2.0*m) + abs(ne + se - 2.0*e);
    let vertical = abs(nw + ne - 2.0*n) + 2.0*abs(w + e - 2.0*m) + abs(sw + se - 2.0*s);
    let along = select(vec2<f32>(0.0, 1.0), vec2<f32>(1.0, 0.0), horizontal >= vertical);
    let normal = along.yx;
    let negative = select(w, n, horizontal >= vertical);
    let positive = select(e, s, horizontal >= vertical);
    let gradient = max(abs(negative - m), abs(positive - m));
    let on_negative = abs(negative - m) >= abs(positive - m);
    let across = select(normal, -normal, on_negative);
    let edge_luma = (m + select(positive, negative, on_negative)) * 0.5;
    let pixel = 1.0 / vec2<f32>(dimensions);
    let uv = (vec2<f32>(p) + 0.5) * pixel;
    let center = uv + 0.5 * across * pixel;
    let increments = array<f32, 8>(1.0, 1.0, 1.5, 2.0, 2.0, 2.0, 4.0, 8.0);
    var distance = vec2<f32>(0.0);
    var ends = vec2<f32>(0.0);
    var found = vec2<bool>(false);
    for (var i = 0; i < 8; i++) {
        if (!found.x) {
            distance.x += increments[i];
            ends.x = luma(textureSampleLevel(scene_color, bilinear, center - along * pixel * distance.x, 0.0).rgb) - edge_luma;
        }
        if (!found.y) {
            distance.y += increments[i];
            ends.y = luma(textureSampleLevel(scene_color, bilinear, center + along * pixel * distance.y, 0.0).rgb) - edge_luma;
        }
        found = abs(ends) >= vec2<f32>(gradient * 0.25);
        if (all(found)) { break; }
    }
    let first = distance.x <= distance.y;
    let endpoint = select(ends.y, ends.x, first);
    let closer_found = select(found.y, found.x, first);
    let across_edge = closer_found && (endpoint < 0.0) != (m < edge_luma);
    let edge_offset = select(0.0, 0.5 - min(distance.x, distance.y) / (distance.x + distance.y), across_edge);
    let neighborhood = (2.0*(n+s+w+e) + nw+ne+sw+se) / 12.0;
    let subpixel = smoothstep(0.0, 1.0, abs(neighborhood - m) / range);
    let offset = max(edge_offset, 0.5 * subpixel * subpixel);
    return textureSampleLevel(scene_color, bilinear, uv + across * pixel * offset, 0.0);
}

const SMAA_THRESHOLD: f32 = 0.10;
const SMAA_SEARCH_STEPS: i32 = 16;
const SMAA_DIAGONAL_STEPS: i32 = 8;
const SMAA_CORNER_ROUNDING: f32 = 0.25;

fn delta(a: vec3<f32>, b: vec3<f32>) -> f32 {
    let d = abs(a - b);
    return max(d.x, max(d.y, d.z));
}

// First pass: west and north edges from gamma-coded colors with local contrast adaptation.
@fragment
fn smaa_edges(in: Vertex) -> @location(0) vec2<f32> {
    let p = vec2<i32>(in.position.xy);
    let center = read_color(p);
    let d = vec2<f32>(delta(center, read_color(p + vec2<i32>(-1,0))),
                      delta(center, read_color(p + vec2<i32>(0,-1))));
    let mask = step(vec2<f32>(SMAA_THRESHOLD), d);
    if (all(mask == vec2<f32>(0))) { discard; }
    let dx = max(delta(center, read_color(p + vec2<i32>( 1,0))), delta(center, read_color(p + vec2<i32>(-2,0))));
    let dy = max(delta(center, read_color(p + vec2<i32>(0, 1))), delta(center, read_color(p + vec2<i32>(0,-2))));
    let contrast = max(max(d.x,d.y), max(dx,dy));
    return mask * step(vec2<f32>(contrast), 2.0 * d);
}

fn edge_at(p: vec2<i32>) -> vec2<f32> {
    return textureLoad(edges_tex, clamp_pixel(p, textureDimensions(edges_tex)), 0).rg;
}

fn edge_sample(p: vec2<f32>) -> vec2<f32> {
    return textureSampleLevel(edges_tex, bilinear, p / vec2<f32>(textureDimensions(edges_tex)), 0.0).rg;
}

fn search_length(edge: vec2<f32>, positive: bool) -> f32 {
    // Original 66x33 domain, remapped to the packed 64x16 texture.
    let bias = vec2<f32>(select(0.5, 33.5, positive), 32.5);
    let uv = (edge * vec2<f32>(32.0, -32.0) + bias) / vec2<f32>(64.0, 16.0);
    return textureSampleLevel(search_tex, bilinear, uv, 0.0).r;
}

fn area(p: vec2<f32>) -> vec2<f32> {
    return textureSampleLevel(area_tex, bilinear, (p + 0.5) / vec2<f32>(textureDimensions(area_tex)), 0.0).rg;
}

fn search_axis(p: vec2<f32>, axis: vec2<f32>, positive: bool, vertical: bool) -> f32 {
    let direction = select(-1.0, 1.0, positive);
    // Fractional positions gather a two-by-two pattern of binary edges.
    var pos = p + select(-0.25, 1.25, positive) * axis - 0.125 * axis.yx;
    var e = vec2<f32>(0.0, 1.0);
    for (var i = 0; i < SMAA_SEARCH_STEPS && e.y > 0.8281 && e.x == 0.0; i++) {
        let v = edge_sample(pos);
        e = select(v, v.yx, vertical);
        pos += direction * 2.0 * axis;
    }
    return dot(pos, axis) - direction * (3.25 - (255.0 / 127.0) * search_length(e, positive));
}

fn crossing(p: vec2<f32>, vertical: bool) -> f32 {
    let e = edge_sample(p);
    return select(e.x, e.y, vertical);
}

fn axis_weights(p: vec2<f32>, vertical: bool) -> vec2<f32> {
    let axis = select(vec2<f32>(1.0, 0.0), vec2<f32>(0.0, 1.0), vertical);
    let minor = axis.yx;
    let left = search_axis(p, axis, false, vertical);
    let right = search_axis(p, axis, true, vertical);
    let d = abs(round(vec2<f32>(left, right) - dot(p, axis)));
    let fixed_coord = dot(p, minor) * minor;
    let a = left*axis + fixed_coord;
    let b = (right + 1.0)*axis + fixed_coord;
    let crosses = vec2<f32>(crossing(a - 0.25*minor, vertical), crossing(b - 0.25*minor, vertical));
    let weights = area(sqrt(d) + 16.0 * round(4.0 * crosses));
    let near = step(d, d.yx);
    let rounding = (1.0 - SMAA_CORNER_ROUNDING) * near / (near.x + near.y);
    let plus = 1.0 - rounding.x*crossing(a + minor, vertical) - rounding.y*crossing(b + minor, vertical);
    let minus = 1.0 - rounding.x*crossing(a - 2.0*minor, vertical) - rounding.y*crossing(b - 2.0*minor, vertical);
    return weights * clamp(vec2<f32>(plus, minus), vec2<f32>(0.0), vec2<f32>(1.0));
}

struct DiagonalEnd {
    distance: i32,
    edge: vec2<f32>,
}

fn diagonal_end(origin: vec2<i32>, dir: vec2<i32>, shifted: bool) -> DiagonalEnd {
    var p = origin;
    var end = DiagonalEnd(-1, vec2<f32>(1.0));
    for (var i = 0; i < SMAA_DIAGONAL_STEPS && end.edge.x + end.edge.y > 1.8; i++) {
        p += dir;
        end.distance += 1;
        end.edge = edge_at(p);
        if (shifted) { end.edge.x = edge_at(p + vec2<i32>(1,0)).x; }
    }
    return end;
}

fn diagonal_weights(p: vec2<i32>, has_west: bool) -> vec2<f32> {
    var weights = vec2<f32>(0.0);
    var a = DiagonalEnd(0, vec2<f32>(0.0));
    if (has_west) {
        a = diagonal_end(p, vec2<i32>(-1,1), false);
        a.distance += i32(a.edge.y > 0.9);
    }
    var b = diagonal_end(p, vec2<i32>(1,-1), false);
    if (a.distance + b.distance > 2) {
        let l = p + vec2<i32>(-a.distance, a.distance);
        let r = p + vec2<i32>(b.distance, -b.distance);
        let e1 = 2.0*edge_at(l + vec2<i32>(-1,0)).y + edge_at(l).x;
        let e2 = 2.0*edge_at(r + vec2<i32>(1,0)).y + edge_at(r + vec2<i32>(1,-1)).x;
        let cross = select(vec2<f32>(e1,e2), vec2<f32>(0.0),
                           vec2<bool>(a.edge.x+a.edge.y >= 1.8, b.edge.x+b.edge.y >= 1.8));
        weights += area(vec2<f32>(80.0,0.0) + vec2<f32>(f32(a.distance),f32(b.distance)) + 20.0*cross);
    }
    a = diagonal_end(p, vec2<i32>(-1,-1), true);
    b = DiagonalEnd(0, vec2<f32>(0.0));
    if (edge_at(p + vec2<i32>(1,0)).x > 0.0) {
        b = diagonal_end(p, vec2<i32>(1,1), true);
        b.distance += i32(b.edge.y > 0.9);
    }
    if (a.distance + b.distance > 2) {
        let l = p - vec2<i32>(a.distance);
        let r = p + vec2<i32>(b.distance);
        let e1 = 2.0*edge_at(l + vec2<i32>(-1,0)).y + edge_at(l + vec2<i32>(0,-1)).x;
        let e = edge_at(r + vec2<i32>(1,0));
        let e2 = 2.0*e.y + e.x;
        let cross = select(vec2<f32>(e1,e2), vec2<f32>(0.0),
                           vec2<bool>(a.edge.x+a.edge.y >= 1.8, b.edge.x+b.edge.y >= 1.8));
        weights += area(vec2<f32>(80.0,0.0) + vec2<f32>(f32(a.distance),f32(b.distance)) + 20.0*cross).yx;
    }
    return weights;
}

// Second pass: pattern searches and reference area weights, with zero subsample offset for 1x.
@fragment
fn smaa_weights(in: Vertex) -> @location(0) vec4<f32> {
    let p = vec2<i32>(in.position.xy);
    let e = edge_at(p);
    var horizontal = vec2<f32>(0.0);
    var vertical = vec2<f32>(0.0);
    if (e.y > 0.0) {
        horizontal = diagonal_weights(p, e.x > 0.0);
        if (any(horizontal > vec2<f32>(0.0))) { return vec4<f32>(horizontal, vertical); }
        horizontal = axis_weights(in.position.xy, false);
    }
    if (e.x > 0.0) { vertical = axis_weights(in.position.xy, true); }
    return vec4<f32>(horizontal, vertical);
}

fn read_weights(p: vec2<i32>) -> vec4<f32> {
    return textureLoad(weights_tex, clamp_pixel(p, textureDimensions(weights_tex)), 0);
}

fn encode_srgb(rgb: vec3<f32>) -> vec3<f32> {
    return select(1.055 * pow(max(rgb, vec3<f32>(0.0)), vec3<f32>(1.0 / 2.4)) - 0.055,
                  12.92 * rgb, rgb <= vec3<f32>(0.0031308));
}

// Third pass: interpolate on the selected axis in linear light. Untouched
// pixels retain their original color codes.
@fragment
fn smaa_neighborhood(in: Vertex) -> @location(0) vec4<f32> {
    let dimensions = textureDimensions(scene_color);
    let p = clamp_pixel(vec2<i32>(in.uv * vec2<f32>(dimensions)), dimensions);
    let w = read_weights(p);
    let horizontal = vec2<f32>(read_weights(p + vec2<i32>(1,0)).a, w.b);
    let vertical = vec2<f32>(read_weights(p + vec2<i32>(0,1)).g, w.r);
    let h = max(horizontal.x,horizontal.y) > max(vertical.x,vertical.y);
    let weights = select(vertical, horizontal, h);
    let total = weights.x + weights.y;
    if (total < 0.00001) { return vec4<f32>(read_color(p), 1.0); }
    let pixel = 1.0 / vec2<f32>(dimensions);
    let uv = (vec2<f32>(p) + 0.5) * pixel;
    let axis = select(vec2<f32>(0.0,1.0), vec2<f32>(1.0,0.0), h) * pixel;
    let a = textureSampleLevel(scene_linear, bilinear, uv + axis*weights.x, 0.0).rgb;
    let b = textureSampleLevel(scene_linear, bilinear, uv - axis*weights.y, 0.0).rgb;
    return vec4<f32>(encode_srgb((a*weights.x + b*weights.y) / total), 1.0);
}

// AMD FidelityFX CAS, CasFilter(noScaling=true), with CAS_SLOW, CAS_GO_SLOWER
// and CAS_BETTER_DIAGONALS. Copyright (c) 2017-2019 Advanced Micro Devices, Inc.
// See cas/LICENSE.txt. Loads use the sRGB view and return linear values.
fn cas_load(p: vec2<i32>, size: vec2<u32>) -> vec3<f32> {
    return textureLoad(cas_source_linear, clamp_pixel(p, size), 0).rgb;
}

@fragment
fn cas_sharpen(in: Vertex) -> @location(0) vec4<f32> {
    let size = textureDimensions(cas_source_linear);
    let p = vec2<i32>(in.uv * vec2<f32>(size));
    let a = cas_load(p + vec2<i32>(-1, -1), size);
    let b = cas_load(p + vec2<i32>( 0, -1), size);
    let c = cas_load(p + vec2<i32>( 1, -1), size);
    let d = cas_load(p + vec2<i32>(-1,  0), size);
    let e = cas_load(p, size);
    let f = cas_load(p + vec2<i32>( 1,  0), size);
    let g = cas_load(p + vec2<i32>(-1,  1), size);
    let h = cas_load(p + vec2<i32>( 0,  1), size);
    let i = cas_load(p + vec2<i32>( 1,  1), size);

    // AMD's soft extrema add the cross to the whole neighborhood.
    let cross_min = min(e, min(min(b,h), min(d,f)));
    let cross_max = max(e, max(max(b,h), max(d,f)));
    let mn = cross_min + min(cross_min, min(min(a,c), min(g,i)));
    let mx = cross_max + max(cross_max, max(max(a,c), max(g,i)));
    let divisor = select(mx, vec3<f32>(1.0), mx == vec3<f32>(0.0));
    let amplitude = sqrt(clamp(min(mn, 2.0 - mx) / divisor, vec3<f32>(0.0), vec3<f32>(1.0)));
    let weight = cas_constants.x * amplitude;
    let sharpened = (e + (b + d + f + h) * weight) / (1.0 + 4.0 * weight);
    return vec4<f32>(encode_srgb(clamp(sharpened, vec3<f32>(0.0), vec3<f32>(1.0))), 1.0);
}
