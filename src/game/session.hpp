#pragma once

// One run through the campaign: the current room (its tiles, crumbling
// blocks, springs, flags and saws), the hero, deaths, checkpoints and time.
// Deterministic: the same per-tick Input from the same starting room gives
// the same run, so a recording replays exactly. No window, audio or clock.

#include "rooms.hpp"
#include "rules.hpp"

#include <gamelib/core/campaign.hpp>
#include <gamelib/core/enemy_motion.hpp>
#include <gamelib/core/event_queue.hpp>
#include <gamelib/core/math.hpp>
#include <gamelib/core/platformer.hpp>
#include <gamelib/core/replay.hpp>
#include <gamelib/core/tile_collision.hpp>
#include <gamelib/core/tile_map.hpp>

#include <cstdint>
#include <vector>

namespace leap::game {

enum class Phase {
    Playing,
    Dying,   // the burst plays; the hero respawns after kDeathTicks
    Exiting, // left the room: waiting for enterNextRoom() (the presentation's wipe)
    Finished // reached the summit beacon
};

// Which border the hero left through.
enum class Side { Left, Right, Up, Down };

enum class EventKind {
    Jump,
    WallJump,
    Land,          // strength: the fall speed
    Spring,
    CrumbleShake,  // cell
    CrumbleBreak,  // cell
    CrumbleReturn, // cell
    Death,
    Respawn,
    Checkpoint,
    RoomExit,      // side
    WorldClear,    // world, ticks, deaths
    Summit,
};

struct Event {
    EventKind kind = EventKind::Jump;
    gamelib::core::Vec2 at;   // world position (the hero's feet, the cell's center...)
    float strength = 0.0f;
    gamelib::core::Cell cell;
    Side side = Side::Right;
    int world = 0;
    int ticks = 0;
    int deaths = 0;
};

struct Crumble {
    gamelib::core::Cell cell;
    enum class State { Solid, Shaking, Gone } state = State::Solid;
    int timer = 0; // ticks left in Shaking or Gone
};

struct Spring {
    gamelib::core::Cell cell;
    int bounce = 0; // ticks since the last launch (for the drawing), large when idle
};

struct Flag {
    gamelib::core::Cell cell;
    bool reached = false;
};

struct Saw {
    gamelib::core::Vec2 position;
    gamelib::core::Vec2 previous;
    float radius = 12.0f;
    float spin = 0.0f; // radians, turns with the distance travelled
    gamelib::core::PathFollower follower;
    std::size_t track = 0; // index into the room's SawTrack list
};

class Session {
public:
    // A run starting at `room` (the first room of a world, or any room for
    // a debug start).
    explicit Session(int room = 0);

    void tick(const Input& input);
    // After a RoomExit event (phase Exiting): the next room. The presentation
    // calls it while its wipe covers the screen.
    void enterNextRoom();

    [[nodiscard]] Phase phase() const { return m_phase; }
    [[nodiscard]] int room() const { return m_room; }
    [[nodiscard]] int world() const { return worldOf(m_room); }
    [[nodiscard]] int startRoom() const { return m_startRoom; }
    [[nodiscard]] const RoomDef& roomDef() const;
    [[nodiscard]] const gamelib::core::TileMap& map() const { return m_map; }
    [[nodiscard]] const gamelib::core::PlatformerBody& hero() const { return m_hero; }
    [[nodiscard]] const gamelib::core::Rect& previousHeroBox() const { return m_previousBox; }
    [[nodiscard]] const std::vector<Crumble>& crumbles() const { return m_crumbles; }
    [[nodiscard]] const std::vector<Spring>& springs() const { return m_springs; }
    [[nodiscard]] const std::vector<Flag>& flags() const { return m_flags; }
    [[nodiscard]] const std::vector<Saw>& saws() const { return m_saws; }
    [[nodiscard]] gamelib::core::Vec2 goal() const { return m_goal; } // the summit beacon (summit room only)
    [[nodiscard]] bool hasGoal() const { return m_hasGoal; }
    [[nodiscard]] Side exitSide() const { return m_exitSide; }
    [[nodiscard]] int dyingTicks() const { return m_dyingTicks; } // ticks since death while Dying

    // The run so far (ticks count while playing and dying, not between rooms).
    [[nodiscard]] int ticks() const { return m_ticks; }
    [[nodiscard]] int deaths() const { return m_deaths; }
    // The current world's share, from entering its first room (or the run's start).
    [[nodiscard]] int worldTicks() const { return m_worldTicks; }
    [[nodiscard]] int worldDeaths() const { return m_worldDeaths; }
    [[nodiscard]] int roomTicks() const { return m_roomTicks; }

    [[nodiscard]] gamelib::core::EventQueue<Event>& events() { return m_events; }
    // A hash of the simulation state, to check that a replay matches.
    [[nodiscard]] std::uint64_t hash() const;

    // The collision world of the current room (for debug drawing and demo checks).
    [[nodiscard]] gamelib::core::TileWorld tileWorld() const;

private:
    void loadRoom(int room);
    void resetRoomObjects();
    void spawnHero(gamelib::core::Vec2 feet);
    void die();
    void stepObjects();
    void touchCrumbles();
    void checkSprings();
    void checkFlags();
    [[nodiscard]] bool hitBySaw() const;
    [[nodiscard]] bool exitAt(Side side) const;

    int m_startRoom = 0;
    int m_room = 0;
    Phase m_phase = Phase::Playing;
    gamelib::core::TileMap m_map{kColumns, kRows, kTile};
    gamelib::core::PlatformerTuning m_tuning;
    gamelib::core::PlatformerBody m_hero;
    gamelib::core::Rect m_previousBox;
    gamelib::core::Checkpoint m_checkpoint;
    std::vector<Crumble> m_crumbles;
    std::vector<Spring> m_springs;
    std::vector<Flag> m_flags;
    std::vector<Saw> m_saws;
    std::vector<bool> m_exits; // per side (Side order): has an E marker on that border
    gamelib::core::Vec2 m_goal;
    bool m_hasGoal = false;
    Side m_exitSide = Side::Right;
    int m_dyingTicks = 0;
    int m_ticks = 0;
    int m_deaths = 0;
    int m_worldTicks = 0;
    int m_worldDeaths = 0;
    int m_roomTicks = 0;
    gamelib::core::EventQueue<Event> m_events;
};

} // namespace leap::game
