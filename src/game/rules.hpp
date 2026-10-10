#pragma once

// Leap's numbers: the room grid, the tick rate, the hero and the movement
// tuning per world. Every rule runs on whole ticks at kTickRate, so the same
// per-tick input always gives the same run (the attract demo is a recording).

#include <gamelib/core/math.hpp>
#include <gamelib/core/platformer.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace leap::game {

inline constexpr int kTickRate = 60;
inline constexpr float kTile = 16.0f; // world units per tile
inline constexpr int kColumns = 32;   // a room is one screen: 32 x 18 tiles
inline constexpr int kRows = 18;
inline constexpr gamelib::core::Vec2 kRoomSize{kColumns * kTile, kRows * kTile};

inline constexpr int kWorlds = 4;
inline constexpr int kRoomsPerWorld = 8;
inline constexpr int kSummitRoom = kWorlds * kRoomsPerWorld; // the last room, after world 4
inline constexpr int kRoomCount = kSummitRoom + 1;

// The hero's collision box (its drawing is a little larger and squashes).
inline constexpr gamelib::core::Vec2 kHeroSize{10.0f, 13.0f};

inline constexpr int kDeathTicks = 24;          // dead -> respawned: 0.4 s
inline constexpr int kCrumbleShakeTicks = 26;   // touched -> falls
inline constexpr int kCrumbleGoneTicks = 150;   // fallen -> comes back (if the hero is not in the way)
inline constexpr float kSpringSpeed = 9.0f;     // upward launch, about 8 tiles high
inline constexpr float kSawHitShrink = 0.75f;   // a saw's hitbox radius is this share of its drawing

// One tick of player input, as recorded for replays: after key mapping,
// never raw keys.
struct Input {
    int move = 0;              // -1 left, 0, 1 right
    bool jumpHeld = false;     // held this tick
    bool jumpPressed = false;  // pressed this tick (also held)

    friend bool operator==(const Input&, const Input&) = default;
};

// Three characters per tick ("R h p"): move L/R/., jump held h/., pressed p/.
[[nodiscard]] std::string encodeInput(const Input& input);
[[nodiscard]] std::optional<Input> decodeInput(std::string_view text);

// Bumped whenever a rule or a room changes, so stale recordings are refused.
inline constexpr std::uint32_t kRulesVersion = 1;

// The movement for a world (0-based; the summit uses the last world's):
// world 1 has no wall slide or wall jump yet, later worlds do.
[[nodiscard]] gamelib::core::PlatformerTuning tuningFor(int world);

} // namespace leap::game
