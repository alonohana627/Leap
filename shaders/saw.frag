#version 450
#include "gamelib/sprite.glsl"
#include "gamelib/sdf.glsl"

// A spinning saw blade: a toothed disc with a hub, drawn as a neon stroke
// over a dark disc. The renderer turns the quad (Sprite::rotation), so the
// teeth spin without the shader knowing the time.
//   params.x  blade radius (to the tooth tips)
//   params.y  number of teeth
//   params.z  glow margin
//   color     blade (darker), color2 neon edge (HDR)

layout(location = 0) in vec2 inUv;
layout(location = 0) out vec4 outColor;

void main()
{
    vec2 p = spriteLocal(inUv);
    float radius = sprite.params.x;
    float teeth = max(sprite.params.y, 3.0);
    float margin = sprite.params.z;

    // Teeth: a sawtooth ripple on the radius (sharp leading edges).
    float angle = atan(p.y, p.x);
    float t = fract(angle / 6.2831853 * teeth);
    float toothDepth = radius * 0.18;
    float edgeRadius = radius - toothDepth * (1.0 - t);
    float d = (length(p) - edgeRadius) * 0.9;

    float hub = length(p) - radius * 0.28;
    float ring = abs(length(p) - radius * 0.62) - 0.6;
    float disc = coverage(d);
    float line = coverage(abs(d + 0.7) - 0.7);
    vec3 rgb = sprite.color.rgb * 0.45;
    rgb = mix(rgb, sprite.color.rgb, coverage(ring) * 0.8);
    rgb = mix(rgb, sprite.color2.rgb * 0.8, coverage(hub));
    rgb = mix(rgb, sprite.color2.rgb, line);

    float halo = 1.0 - clamp(max(d, 0.0) / max(margin, 1e-3), 0.0, 1.0);
    outColor = over(vec4(rgb, disc), vec4(sprite.color2.rgb, halo * halo * 0.5 * step(0.0, d)));
}
