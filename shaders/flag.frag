#version 450
#include "gamelib/sprite.glsl"
#include "gamelib/sdf.glsl"

// A checkpoint flag: a thin pole with a pennant that waves in the wind, or
// (params.z = 1) the summit beacon, a pillar of light with a turning crystal.
//   params.x  raised 0..1 (the pennant climbs the pole when reached)
//   params.y  wave phase
//   params.z  0 flag, 1 beacon
//   color     pennant / crystal, color2 glow (HDR)

layout(location = 0) in vec2 inUv;
layout(location = 0) out vec4 outColor;

void main()
{
    vec2 p = spriteLocal(inUv);
    vec2 h = sprite.size * 0.5;
    float t = sprite.time;

    if (sprite.params.z > 0.5) {
        // The beacon: a soft vertical beam and a diamond bobbing in it.
        float beam = exp(-abs(p.x) / (h.x * 0.18)) * smoothstep(h.y, -h.y, p.y) * (0.75 + 0.25 * sin(t * 3.0));
        vec2 c = vec2(0.0, h.y * 0.35 + sin(t * 2.0) * 2.0);
        vec2 q = p - c;
        float spin = abs(cos(t * 1.4));
        float diamond = (abs(q.x) / max(spin, 0.15) + abs(q.y)) - 6.0;
        float body = coverage(diamond);
        float edge = coverage(abs(diamond + 0.6) - 0.6);
        vec4 crystal = vec4(mix(sprite.color.rgb, sprite.color2.rgb, edge), body);
        float halo = 1.0 - clamp(length(q) / (h.x * 0.9), 0.0, 1.0);
        vec4 glowColor = vec4(sprite.color2.rgb, clamp(beam * 0.35 + halo * halo * 0.5, 0.0, 1.0));
        outColor = over(crystal, glowColor);
        return;
    }

    // The pole along the left third, from the bottom to near the top.
    float poleX = -h.x * 0.45;
    float pole = sdSegment(p, vec2(poleX, h.y), vec2(poleX, -h.y + 2.0), 0.8);
    float knob = length(p - vec2(poleX, -h.y + 2.0)) - 1.6;

    // The pennant: a triangle whose rows sway with a travelling wave.
    float raised = clamp(sprite.params.x, 0.0, 1.0);
    float top = mix(h.y * 0.25, -h.y + 3.0, raised);
    float flagH = h.y * 0.55;
    float length_ = h.x * 1.2;
    float u = clamp((p.x - poleX) / length_, 0.0, 1.0);
    float wave = sin(u * 5.0 - t * 6.0 + sprite.params.y) * 1.6 * u;
    float y = p.y - top - wave;
    float halfHeight = flagH * 0.5 * (1.0 - u);
    float pennant = max(abs(y - flagH * 0.5) - halfHeight, max(poleX - p.x, p.x - (poleX + length_)));

    float d = min(min(pole, knob), pennant);
    vec3 rgb = mix(vec3(0.75, 0.8, 0.9), sprite.color.rgb * (0.8 + 0.4 * u), step(pennant, min(pole, knob)));
    float line = coverage(abs(pennant + 0.5) - 0.5);
    rgb = mix(rgb, sprite.color2.rgb, line * 0.7);
    float halo = 1.0 - clamp(max(d, 0.0) / 4.0, 0.0, 1.0);
    outColor = over(vec4(rgb, coverage(d)), vec4(sprite.color2.rgb, halo * halo * (0.2 + 0.3 * raised) * step(0.0, d)));
}
