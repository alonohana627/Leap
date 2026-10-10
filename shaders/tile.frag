#version 450
#include "gamelib/sprite.glsl"
#include "gamelib/sdf.glsl"
#include "gamelib/noise.glsl"

// Leap's one tile shader: every kind of tile in a room, neon on dark.
//   params.x  kind: 0 rock, 1-4 spikes pointing up/down/left/right,
//             5 crumbling block, 6 spring
//   params.y  edge line width (world units)
//   params.z  corner radius of open outer corners
//   params.w  rock: unused; crumble: crack amount 0..1; spring: compression 0..1
//   color     fill (rock, crumble) or body (spikes, spring)
//   color2    the neon edge (HDR, bloomed)
//   bits      0-7 neighbour mask (bit set = same kind there), 8-15 variant,
//             32-47 cell x, 48-63 cell y (draw::drawTiles' layout)
// Rounded edges only where a neighbour is empty: connected sides run past the
// quad edge, so walls join without seams.

layout(location = 0) in vec2 inUv;
layout(location = 0) out vec4 outColor;

bool connected(uint mask, uint bit)
{
    return (mask & bit) != 0u;
}

float sdTriangle(vec2 p, vec2 a, vec2 b, vec2 c)
{
    vec2 e0 = b - a, e1 = c - b, e2 = a - c;
    vec2 v0 = p - a, v1 = p - b, v2 = p - c;
    vec2 pq0 = v0 - e0 * clamp(dot(v0, e0) / dot(e0, e0), 0.0, 1.0);
    vec2 pq1 = v1 - e1 * clamp(dot(v1, e1) / dot(e1, e1), 0.0, 1.0);
    vec2 pq2 = v2 - e2 * clamp(dot(v2, e2) / dot(e2, e2), 0.0, 1.0);
    float s = sign(e0.x * e2.y - e0.y * e2.x);
    vec2 d = min(min(vec2(dot(pq0, pq0), s * (v0.x * e0.y - v0.y * e0.x)),
                     vec2(dot(pq1, pq1), s * (v1.x * e1.y - v1.y * e1.x))),
                 vec2(dot(pq2, pq2), s * (v2.x * e2.y - v2.y * e2.x)));
    return -sqrt(d.x) * sign(d.y);
}

// A block that knows its neighbours: fill, a neon line on open sides, a soft
// glow inside it. Returns straight-alpha color.
vec4 block(vec2 p, vec2 h, uint mask, float variant, float edge, float radius, float cracks)
{
    bool n = connected(mask, 1u);
    bool e = connected(mask, 2u);
    bool s = connected(mask, 4u);
    bool w = connected(mask, 8u);
    vec2 lo = vec2(w ? -2.0 * h.x : -h.x, n ? -2.0 * h.y : -h.y);
    vec2 hi = vec2(e ? 2.0 * h.x : h.x, s ? 2.0 * h.y : h.y);
    float r = min(radius, min(h.x, h.y));
    float d = sdRoundBox(p - (lo + hi) * 0.5, (hi - lo) * 0.5, r);

    // Inner corners (both sides connected, the diagonal open) get a small
    // rounded notch of edge light, so concave corners read too.
    float notch = 1e5;
    if (n && e && !connected(mask, 16u)) notch = min(notch, length(p - vec2(h.x, -h.y)));
    if (s && e && !connected(mask, 32u)) notch = min(notch, length(p - vec2(h.x, h.y)));
    if (s && w && !connected(mask, 64u)) notch = min(notch, length(p - vec2(-h.x, h.y)));
    if (n && w && !connected(mask, 128u)) notch = min(notch, length(p - vec2(-h.x, -h.y)));

    float shape = coverage(d);
    float line = coverage(abs(d + edge * 0.5) - edge * 0.5);
    float notchLine = coverage(abs(notch - edge * 1.5) - edge * 0.5);
    // Light leaking inwards from the open edges only: the distance to a box
    // pushed far past every connected side, so the glow is continuous from
    // one tile to the next instead of outlining every tile.
    vec2 farLo = vec2(w ? -100.0 * h.x : -h.x, n ? -100.0 * h.y : -h.y);
    vec2 farHi = vec2(e ? 100.0 * h.x : h.x, s ? 100.0 * h.y : h.y);
    float dOpen = sdRoundBox(p - (farLo + farHi) * 0.5, (farHi - farLo) * 0.5, r);
    float inner = max(exp(dOpen / (edge * 3.0)), exp(-notch / (edge * 3.0))) * step(d, 0.0);

    // Fill: dark, with broad wavy strata and a fine grain, both in world space.
    vec2 cell = vec2(float(sprite.bits.y & 0xffffu), float(sprite.bits.y >> 16));
    vec2 world = cell * (2.0 * h) + p + h;
    float strata = 0.5 + 0.5 * sin((world.x * 0.6 + world.y) * 0.09 + 2.0 * valueNoise(world * 0.04));
    float speckle = valueNoise(world * 0.45); // in world space: continuous across tiles
    vec3 fill = sprite.color.rgb * (0.8 + 0.25 * strata + 0.1 * speckle);

    // Cracks for crumbling blocks: Voronoi cell borders, widening as it breaks.
    if (cracks > 0.0) {
        vec3 v = voronoi(world * 0.28);
        float crack = 1.0 - smoothstep(0.0, 0.05 + 0.12 * cracks, v.y - v.x);
        fill = mix(fill, sprite.color2.rgb * 0.6, crack * (0.35 + 0.65 * cracks));
    }

    vec3 rgb = fill + sprite.color2.rgb * inner * 0.22;
    rgb = mix(rgb, sprite.color2.rgb, max(line, notchLine * 0.8));
    return vec4(rgb, sprite.color.a * shape);
}

vec4 spikes(vec2 p, vec2 h, int dir)
{
    // Rotate so the spikes point up (-y): base on the +y side of the tile.
    vec2 q = p;
    if (dir == 2) q = vec2(-p.x, -p.y);
    else if (dir == 3) q = vec2(p.y, -p.x);
    else if (dir == 4) q = vec2(-p.y, p.x);
    float d = 1e5;
    float tipY = -h.y + 2.5;
    for (int i = 0; i < 2; ++i) {
        float cx = -h.x * 0.5 + float(i) * h.x;
        d = min(d, sdTriangle(q, vec2(cx - h.x * 0.5, h.y), vec2(cx + h.x * 0.5, h.y), vec2(cx, tipY)));
    }
    float body = coverage(d);
    float line = coverage(abs(d + 0.6) - 0.6);
    float glowAmount = (1.0 - clamp(max(d, 0.0) / 3.0, 0.0, 1.0));
    vec3 rgb = mix(sprite.color.rgb * (0.55 + 0.45 * (h.y - q.y) / (2.0 * h.y)), sprite.color2.rgb, line);
    vec4 shape = vec4(rgb, body);
    vec4 halo = vec4(sprite.color2.rgb, glowAmount * glowAmount * 0.35 * step(0.0, d));
    return over(shape, halo);
}

vec4 spring(vec2 p, vec2 h, float squash)
{
    // A base plate on the floor, a zigzag coil and a pad that drops when it fires.
    float padY = mix(-h.y * 0.15, h.y * 0.35, squash);
    float base = sdRoundBox(p - vec2(0.0, h.y - 1.5), vec2(h.x * 0.8, 1.5), 1.0);
    float pad = sdRoundBox(p - vec2(0.0, padY - 1.5), vec2(h.x * 0.85, 1.6), 1.2);
    float coil = 1e5;
    float top = padY;
    float bottom = h.y - 3.0;
    for (int i = 0; i < 4; ++i) {
        float y0 = mix(top, bottom, float(i) / 4.0);
        float y1 = mix(top, bottom, float(i + 1) / 4.0);
        float x0 = (i % 2 == 0) ? -h.x * 0.55 : h.x * 0.55;
        coil = min(coil, sdSegment(p, vec2(x0, y0), vec2(-x0, y1), 0.9));
    }
    float d = min(min(base, pad), coil);
    float line = coverage(d);
    vec3 rgb = mix(sprite.color.rgb, sprite.color2.rgb, step(pad, 0.0) * 0.6 + 0.2);
    float halo = (1.0 - clamp(max(d, 0.0) / 3.0, 0.0, 1.0));
    return over(vec4(rgb, line), vec4(sprite.color2.rgb, halo * halo * 0.3 * step(0.0, d)));
}

void main()
{
    vec2 p = spriteLocal(inUv);
    vec2 h = sprite.size * 0.5;
    int kind = int(sprite.params.x + 0.5);
    uint mask = sprite.bits.x & 0xffu;
    float variant = float((sprite.bits.x >> 8) & 0xffu);

    if (kind >= 1 && kind <= 4) {
        outColor = spikes(p, h, kind);
    } else if (kind == 6) {
        outColor = spring(p, h, sprite.params.w);
    } else {
        float cracks = kind == 5 ? 0.15 + 0.85 * sprite.params.w : 0.0;
        outColor = block(p, h, mask, variant, sprite.params.y, sprite.params.z, cracks);
    }
}
