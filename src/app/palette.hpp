#pragma once

// Leap's colors: neon on dark. Each world has its own background, rock and
// edge hues; the hero, hazards, springs and flags keep one color everywhere
// so a danger always reads the same. Edge colors go above 1.0 so the bloom
// pass makes them glow.

#include <gamelib/core/color.hpp>

#include <array>
#include <cstddef>

namespace leap::app {

using gamelib::core::Color;
using gamelib::core::srgb;

struct WorldPalette {
    Color sky;      // background, top
    Color skyLow;   // background, bottom
    Color haze;     // background shapes
    Color rock;     // tile fill
    Color edge;     // tile outline (HDR)
    Color accent;   // background motes, the HUD's world name
};

[[nodiscard]] inline Color glow(const Color c, const float strength)
{
    return {c.r * strength, c.g * strength, c.b * strength, c.a};
}

// 0 Foothills, 1 Cliffs, 2 Quarry, 3 Foundry, 4 Summit.
[[nodiscard]] inline const WorldPalette& worldPalette(const int world)
{
    static const std::array<WorldPalette, 5> kPalettes{{
        {srgb(0x07141C), srgb(0x0E2630), srgb(0x14404A), srgb(0x0F2430), glow(srgb(0x3DF5C4), 1.6f), srgb(0x3DF5C4)},
        {srgb(0x080B22), srgb(0x131A44), srgb(0x22306E), srgb(0x141A3E), glow(srgb(0x63B8FF), 1.6f), srgb(0x63B8FF)},
        {srgb(0x160C06), srgb(0x2A170B), srgb(0x4A2A12), srgb(0x2A1A0E), glow(srgb(0xFFB347), 1.5f), srgb(0xFFB347)},
        {srgb(0x14060F), srgb(0x2A0C20), srgb(0x4A1238), srgb(0x2A0E24), glow(srgb(0xFF5AB8), 1.6f), srgb(0xFF5AB8)},
        {srgb(0x0A0820), srgb(0x1C1640), srgb(0x3A2A70), srgb(0x1E1A44), glow(srgb(0xF6E7A8), 1.5f), srgb(0xF6E7A8)},
    }};
    return kPalettes[static_cast<std::size_t>(world < 0 ? 0 : (world > 4 ? 4 : world))];
}

inline const Color kHero = srgb(0xFFE45C);
inline const Color kHeroEdge = glow(srgb(0xFFF3B0), 1.4f);
inline const Color kHazard = srgb(0xFF3D5A);
inline const Color kHazardGlow = glow(srgb(0xFF4D6A), 1.8f);
inline const Color kSaw = srgb(0xFF6A3D);
inline const Color kSawGlow = glow(srgb(0xFF8A4D), 1.8f);
inline const Color kSpring = srgb(0x7CFF6B);
inline const Color kSpringGlow = glow(srgb(0x9CFF8B), 1.6f);
inline const Color kFlag = srgb(0x5CE1FF);
inline const Color kFlagReached = srgb(0xFFE45C);
inline const Color kText = srgb(0xF2F0E5);
inline const Color kTextDim = srgb(0x9AA3BF);
inline const Color kInk = srgb(0x05070F); // eyes, transition cover

} // namespace leap::app
