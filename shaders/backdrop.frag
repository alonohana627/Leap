#version 450
#include "gamelib/sprite.glsl"
#include "gamelib/sdf.glsl"
#include "gamelib/noise.glsl"

// The animated backdrop behind a room, one look per world, drawn over a
// plain gradient as a translucent layer: far silhouettes in the haze color
// and slow motes or lights in the accent color. Subtle on purpose: the room
// in front must stay readable.
//   params.x  world: 0 Foothills, 1 Cliffs, 2 Quarry, 3 Foundry, 4 Summit
//   params.y  extra brightness 0..1 (the title screen shows it brighter)
//   color     haze (silhouettes), color2 accent (motes, aurora, sparks)

layout(location = 0) in vec2 inUv;
layout(location = 0) out vec4 outColor;

// Soft round motes on a hashed grid that drift with time; each cell holds at
// most one mote, kept inside its cell so nothing is cut at a cell edge.
float motes(vec2 p, float cell, vec2 drift, float time, float density)
{
    vec2 q = p + drift * time;
    vec2 id = floor(q / cell);
    vec2 f = fract(q / cell) - 0.5;
    float h = hash12(uvec2(ivec2(id) + 4096));
    if (h > density) {
        return 0.0;
    }
    vec2 offset = vec2(hash12(uvec2(ivec2(id) + 777)), hash12(uvec2(ivec2(id) + 1999))) - 0.5;
    float r = 0.06 + 0.06 * hash12(uvec2(ivec2(id) + 31));
    float d = length(f - offset * 0.5) / r;
    float twinkle = 0.6 + 0.4 * sin(time * (1.0 + 2.0 * h) + h * 40.0);
    return clamp(1.0 - d, 0.0, 1.0) * twinkle;
}

// A ridge line: 1 below the height field (in canvas units from the top).
float ridge(vec2 p, float base, float amplitude, float scale, float seed)
{
    float height = base - amplitude * fbm(vec2(p.x / scale + seed, seed * 3.1), 4);
    return smoothstep(height - 1.5, height + 1.5, p.y);
}

void main()
{
    vec2 p = inUv * sprite.size;
    vec2 size = sprite.size;
    float t = sprite.time;
    int world = int(sprite.params.x + 0.5);
    float bright = 1.0 + sprite.params.y;
    vec3 haze = sprite.color.rgb;
    vec3 accent = sprite.color2.rgb;

    vec4 result = vec4(0.0);
    if (world == 0) {
        // Foothills: two ranges of rolling hills and a slow aurora ribbon.
        float far = ridge(p + vec2(t * 6.0, 0.0), size.y * 0.62, 220.0, 420.0, 1.0);
        float near = ridge(p + vec2(t * 14.0, 0.0), size.y * 0.80, 160.0, 300.0, 7.0);
        float band = exp(-pow((p.y - size.y * 0.22 - 60.0 * sin(p.x / 260.0 + t * 0.15)) / 70.0, 2.0));
        float aurora = band * (0.5 + 0.5 * fbm(vec2(p.x / 180.0 + t * 0.05, t * 0.03), 3));
        result = over(vec4(haze * 1.25, near * 0.55), vec4(haze, far * 0.35));
        result = over(result, vec4(accent, aurora * 0.10));
        result = over(vec4(accent, motes(p, 90.0, vec2(-8.0, -5.0), t, 0.3) * 0.35), result);
    } else if (world == 1) {
        // Cliffs: tall stepped rock columns and thin falling streaks.
        float column = floor((p.x + t * 4.0) / 70.0);
        float top = size.y * (0.25 + 0.45 * hash12(uvec2(uint(column + 500.0), 3u)));
        float cliff = smoothstep(top - 1.5, top + 1.5, p.y);
        float column2 = floor((p.x + t * 10.0) / 110.0);
        float top2 = size.y * (0.45 + 0.4 * hash12(uvec2(uint(column2 + 900.0), 5u)));
        float cliff2 = smoothstep(top2 - 1.5, top2 + 1.5, p.y);
        vec2 r = vec2(p.x + p.y * 0.12, p.y - t * 320.0);
        float streakId = floor(r.x / 23.0);
        float streak = step(0.93, hash12(uvec2(uint(streakId + 4000.0), uint(floor(r.y / 260.0)) + 9u)));
        float streakShape = streak * smoothstep(0.0, 0.4, fract(r.y / 260.0)) * (1.0 - smoothstep(0.4, 0.5, fract(r.y / 260.0)))
                            * (1.0 - smoothstep(0.0, 1.2, abs(fract(r.x / 23.0) - 0.5) * 23.0));
        result = over(vec4(haze * 1.3, cliff2 * 0.5), vec4(haze, cliff * 0.3));
        result = over(vec4(accent, streakShape * 0.18), result);
    } else if (world == 2) {
        // Quarry: wavy strata bands and dust rising in the warm air.
        float strata = sin(p.y / 38.0 + 1.5 * fbm(vec2(p.x / 300.0 + t * 0.02, p.y / 200.0), 3) * 3.0);
        float bands = smoothstep(0.55, 0.75, strata) * smoothstep(size.y * 0.15, size.y * 0.9, p.y);
        result = vec4(haze, bands * 0.35);
        result = over(vec4(accent, motes(p, 70.0, vec2(4.0, 22.0), t, 0.35) * 0.4), result);
    } else if (world == 3) {
        // Foundry: girders and pipes, a heat glow from below, sparks rising.
        float beam = step(0.82, fract((p.x + 40.0) / 240.0)) + step(0.9, fract(p.y / 300.0 + 0.3)) * 0.6;
        float pipe = 1.0 - smoothstep(0.0, 2.0, abs(fract((p.x + 130.0) / 360.0) - 0.5) * 360.0 - 12.0);
        float pulse = 0.5 + 0.5 * sin(t * 1.3);
        float heat = smoothstep(size.y * 0.55, size.y, p.y) * (0.6 + 0.4 * pulse)
                     * (0.6 + 0.4 * fbm(vec2(p.x / 160.0, t * 0.4), 3));
        result = vec4(haze * 1.2, clamp(beam, 0.0, 1.0) * 0.3 + pipe * 0.25);
        result = over(vec4(accent, heat * 0.14), result);
        result = over(vec4(accent * 1.3, motes(p, 60.0, vec2(-6.0, 48.0), t, 0.18) * 0.55), result);
    } else {
        // Summit: a starry sky with a wide aurora curtain.
        float stars = motes(p, 46.0, vec2(1.0, 0.0), t, 0.45);
        float curtain = exp(-pow((p.y - size.y * 0.3 - 90.0 * sin(p.x / 340.0 + t * 0.2)) / 140.0, 2.0));
        float folds = 0.8 + 0.2 * sin(p.x / 60.0 + 3.0 * fbm(vec2(p.x / 220.0, t * 0.1), 3));
        float range = ridge(p, size.y * 0.86, 200.0, 360.0, 11.0);
        result = vec4(accent, curtain * folds * 0.07);
        result = over(vec4(vec3(1.0), stars * 0.7), result);
        result = over(vec4(haze, range * 0.6), result);
    }
    result.rgb *= bright;
    result.a = clamp(result.a * (0.85 + 0.15 * bright), 0.0, 1.0);
    outColor = vec4(dither(result.rgb, gl_FragCoord.xy, 1.0 / 255.0), result.a);
}
