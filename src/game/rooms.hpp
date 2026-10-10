#pragma once

// The campaign's rooms as data: text grids, one character per tile, read
// with core::TileMap::fromText, plus the saws' fixed tracks and a hint line.
//
//   #  solid rock            C  crumbling block (falls a moment after being touched, comes back)
//   ^ v < >  spikes (pointing up, down, left, right; hitbox smaller than the drawing)
//   Y  spring (launches upwards)
//   S  start: where the hero enters the room (standing on the tile below)
//   F  checkpoint flag       E  exit (a gap in the border: leaving through it goes on)
//   G  the summit beacon (the end of the campaign)
//
// Rooms 0-7 are world 1, 8-15 world 2, 16-23 world 3, 24-31 world 4, 32 the summit.

#include "rules.hpp"

#include <gamelib/core/math.hpp>

#include <array>
#include <span>
#include <string_view>
#include <vector>

namespace leap::game {

// A saw moving along a fixed polyline of tile coordinates (cell centers;
// fractions allowed), ping-ponging or looping, at `speed` world units per tick.
struct SawTrack {
    std::vector<gamelib::core::Vec2> points; // in tiles
    float speed = 1.0f;
    float radius = 12.0f; // world units (drawn size)
    bool loop = false;    // false: back and forth; true: round the closed polyline
    float startOffset = 0.0f; // world units along the track at room entry
};

struct RoomDef {
    std::string_view name;
    std::array<std::string_view, kRows> rows;
    std::vector<SawTrack> saws;
    std::string_view hint;          // a line drawn in the room (teaching), may be empty
    gamelib::core::Vec2 hintAt;     // in tiles: the hint's center
};

[[nodiscard]] std::span<const RoomDef> rooms();
[[nodiscard]] constexpr int worldOf(const int room) { return room / kRoomsPerWorld; }
[[nodiscard]] constexpr int firstRoomOf(const int world) { return world * kRoomsPerWorld; }
// "FOOTHILLS", ... and "SUMMIT" for world 4 (the summit room).
[[nodiscard]] std::string_view worldName(int world);

} // namespace leap::game
