#version 450
#include "gamelib/sprite.glsl"
#include "gamelib/sdf.glsl"

// The hero: a soft blob with two eyes. The quad is the body (already
// squashed or stretched by the caller) plus a glow margin.
//   params.x  facing: -1 left, 1 right (the eyes look that way)
//   params.y  blink 0..1 (1 = closed)
//   params.z  vertical look -1 (up) .. 1 (down), from the vertical speed
//   params.w  glow margin around the body (sprite units)
//   color     body, color2 rim light (HDR)

layout(location = 0) in vec2 inUv;
layout(location = 0) out vec4 outColor;

void main()
{
    vec2 p = spriteLocal(inUv);
    float margin = sprite.params.w;
    vec2 h = sprite.size * 0.5 - vec2(margin);
    float facing = sprite.params.x;

    // A rounded box whose top is rounder than its feet: a gumdrop.
    float radius = min(h.x, h.y) * mix(0.55, 0.85, step(p.y, 0.0));
    float d = sdRoundBox(p, h, radius);
    float body = coverage(d);

    // Shading: lighter at the top, a rim of light on the edge.
    float shade = 0.82 + 0.3 * clamp(-p.y / h.y, -1.0, 1.0);
    vec3 rgb = sprite.color.rgb * shade;
    float rim = coverage(abs(d + 0.6) - 0.6);
    rgb = mix(rgb, sprite.color2.rgb, rim * 0.85);

    // Eyes: white ovals with dark pupils, shifted towards the facing side.
    vec2 eyeCenter = vec2(facing * h.x * 0.28, -h.y * 0.18 + sprite.params.z * h.y * 0.08);
    float spacing = h.x * 0.36;
    float blink = clamp(sprite.params.y, 0.0, 1.0);
    vec2 eyeSize = vec2(h.x * 0.17, max(h.y * 0.22 * (1.0 - blink), 0.25));
    float eyes = 1e5;
    float pupils = 1e5;
    for (int i = -1; i <= 1; i += 2) {
        vec2 c = eyeCenter + vec2(float(i) * spacing * 0.5, 0.0);
        eyes = min(eyes, sdEllipse(p - c, eyeSize));
        vec2 pc = c + vec2(facing * eyeSize.x * 0.35, sprite.params.z * eyeSize.y * 0.3);
        pupils = min(pupils, sdEllipse(p - pc, eyeSize * vec2(0.55, 0.6)));
    }
    rgb = mix(rgb, vec3(1.0), coverage(eyes));
    rgb = mix(rgb, vec3(0.02, 0.02, 0.05), coverage(max(pupils, eyes)));

    // A glow around the body that ends exactly at the quad edge.
    float halo = 1.0 - clamp(max(d, 0.0) / max(margin, 1e-3), 0.0, 1.0);
    vec4 glowColor = vec4(sprite.color2.rgb, halo * halo * 0.45 * step(0.0, d));
    outColor = over(vec4(rgb, body * sprite.color.a), glowColor * vec4(1.0, 1.0, 1.0, sprite.color.a));
}
