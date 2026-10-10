#include "session.hpp"

#include <gamelib/core/collision.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>

namespace leap::game {

namespace {

// helpers

using gamelib::core::Cell;
using gamelib::core::Rect;
using gamelib::core::TileShape;
using gamelib::core::Vec2;

constexpr char kCrumbleGone = 'c';

const gamelib::core::TileShapes& _shapes()
{
    static const gamelib::core::TileShapes shapes{
        {'#', TileShape::Solid}, {'C', TileShape::Solid},  {'^', TileShape::Hazard},
        {'v', TileShape::Hazard}, {'<', TileShape::Hazard}, {'>', TileShape::Hazard},
    };
    return shapes;
}

Vec2 _tilesToWorld(const Vec2 tiles)
{
    return {(tiles.x + 0.5f) * kTile, (tiles.y + 0.5f) * kTile};
}

// The spring's trigger: the lower part of its cell, a little narrower.
Rect _springRect(const Cell cell)
{
    const Rect r = gamelib::core::cellRect(cell, kTile);
    return {r.x + 2.0f, r.y + 9.0f, r.width - 4.0f, r.height - 9.0f};
}

} // namespace

Session::Session(const int room)
    : m_startRoom{room}
{
    loadRoom(room);
}

const RoomDef& Session::roomDef() const
{
    return rooms()[static_cast<std::size_t>(m_room)];
}

gamelib::core::TileWorld Session::tileWorld() const
{
    return gamelib::core::TileWorld{m_map, 0, _shapes()};
}

void Session::loadRoom(const int room)
{
    if (room < 0 || room >= kRoomCount) {
        throw std::out_of_range{"Leap: no such room"};
    }
    m_room = room;
    m_tuning = tuningFor(std::min(worldOf(room), kWorlds - 1));
    const RoomDef& def = roomDef();
    const std::array<gamelib::core::LayerMapping, 1> layers{gamelib::core::charTile};
    m_map = gamelib::core::TileMap::fromText(def.rows, kTile, layers, "SFEG");

    m_exits.assign(4, false);
    m_hasGoal = false;
    for (const gamelib::core::TileMarker& marker : m_map.markers("EG")) {
        if (marker.symbol == 'G') {
            m_goal = m_map.cellCenter(marker.cell);
            m_hasGoal = true;
            continue;
        }
        if (marker.cell.x == 0) m_exits[static_cast<std::size_t>(Side::Left)] = true;
        if (marker.cell.x == kColumns - 1) m_exits[static_cast<std::size_t>(Side::Right)] = true;
        if (marker.cell.y == 0) m_exits[static_cast<std::size_t>(Side::Up)] = true;
        if (marker.cell.y == kRows - 1) m_exits[static_cast<std::size_t>(Side::Down)] = true;
    }
    m_flags.clear();
    for (const gamelib::core::TileMarker& marker : m_map.markers("F")) {
        m_flags.push_back({.cell = marker.cell});
    }
    resetRoomObjects();

    const std::vector<gamelib::core::TileMarker> starts = m_map.markers("S");
    if (starts.empty()) {
        throw std::logic_error{"Leap: a room without a start"};
    }
    const Vec2 entry = m_map.cellBottom(starts.front().cell);
    m_checkpoint.enterRoom(room, entry);
    m_roomTicks = 0;
    spawnHero(entry);
    m_phase = Phase::Playing;
}

void Session::resetRoomObjects()
{
    // Fallen blocks come back and springs rest: a respawn sees the room as it was.
    m_crumbles.clear();
    m_springs.clear();
    gamelib::core::TileLayer& layer = m_map.layer(0);
    for (int y = 0; y < layer.height(); ++y) {
        for (int x = 0; x < layer.width(); ++x) {
            const Cell cell{x, y};
            gamelib::core::TileId& id = layer.at(cell);
            if (id == kCrumbleGone) {
                id = 'C';
            }
            if (id == 'C') {
                m_crumbles.push_back({.cell = cell});
            } else if (id == 'Y') {
                m_springs.push_back({.cell = cell, .bounce = 1000});
            }
        }
    }
    // Saws restart from their starting points, so every attempt meets them the same way.
    m_saws.clear();
    const RoomDef& def = roomDef();
    for (std::size_t i = 0; i < def.saws.size(); ++i) {
        const SawTrack& track = def.saws[i];
        std::vector<Vec2> points;
        for (const Vec2 p : track.points) {
            points.push_back(_tilesToWorld(p));
        }
        Saw saw{.position = points.front(), .previous = points.front(), .radius = track.radius,
                .follower = {.target = points.size() > 1 ? 1u : 0u,
                             .end = track.loop ? gamelib::core::Loop::Repeat : gamelib::core::Loop::PingPong},
                .track = i};
        if (track.loop) {
            points.push_back(points.front()); // round the closed polyline
        }
        saw.position = gamelib::core::followPath(saw.position, points, saw.follower, track.startOffset);
        saw.previous = saw.position;
        m_saws.push_back(saw);
    }
}

void Session::spawnHero(const Vec2 feet)
{
    m_hero = {};
    m_hero.box = Rect{feet.x - kHeroSize.x / 2.0f, feet.y - kHeroSize.y, kHeroSize.x, kHeroSize.y};
    m_hero.grounded = true;
    m_previousBox = m_hero.box;
}

void Session::enterNextRoom()
{
    if (m_phase != Phase::Exiting) {
        return;
    }
    const int next = std::min(m_room + 1, kRoomCount - 1);
    if (worldOf(next) != worldOf(m_room)) {
        m_worldTicks = 0;
        m_worldDeaths = 0;
    }
    loadRoom(next);
}

void Session::die()
{
    m_phase = Phase::Dying;
    m_dyingTicks = 0;
    ++m_deaths;
    ++m_worldDeaths;
    // A fall into a pit bursts at the bottom edge, where it can be seen.
    const Vec2 at = m_hero.box.center();
    m_events.push({.kind = EventKind::Death, .at = {at.x, std::min(at.y, kRoomSize.y - 6.0f)}});
}

void Session::stepObjects()
{
    const RoomDef& def = roomDef();
    for (Saw& saw : m_saws) {
        const SawTrack& track = def.saws[saw.track];
        std::vector<Vec2> points;
        for (const Vec2 p : track.points) {
            points.push_back(_tilesToWorld(p));
        }
        if (track.loop) {
            points.push_back(points.front());
        }
        saw.previous = saw.position;
        saw.position = gamelib::core::followPath(saw.position, points, saw.follower, track.speed);
        saw.spin += track.speed / saw.radius;
    }
    gamelib::core::TileLayer& layer = m_map.layer(0);
    for (Crumble& c : m_crumbles) {
        switch (c.state) {
        case Crumble::State::Solid: break;
        case Crumble::State::Shaking:
            if (--c.timer <= 0) {
                c.state = Crumble::State::Gone;
                c.timer = kCrumbleGoneTicks;
                layer.at(c.cell) = kCrumbleGone;
                m_events.push({.kind = EventKind::CrumbleBreak, .at = m_map.cellCenter(c.cell), .cell = c.cell});
            }
            break;
        case Crumble::State::Gone:
            if (c.timer > 0) {
                --c.timer;
            }
            // Back only when nobody stands where it would appear.
            if (c.timer == 0 && !gamelib::core::overlaps(gamelib::core::cellRect(c.cell, kTile), m_hero.box)) {
                c.state = Crumble::State::Solid;
                layer.at(c.cell) = 'C';
                m_events.push({.kind = EventKind::CrumbleReturn, .at = m_map.cellCenter(c.cell), .cell = c.cell});
            }
            break;
        }
    }
    for (Spring& s : m_springs) {
        s.bounce = std::min(s.bounce + 1, 1000);
    }
}

void Session::touchCrumbles()
{
    const Rect& box = m_hero.box;
    const auto trigger = [&](const Cell cell) {
        for (Crumble& c : m_crumbles) {
            if (c.cell == cell && c.state == Crumble::State::Solid) {
                c.state = Crumble::State::Shaking;
                c.timer = kCrumbleShakeTicks;
                m_events.push({.kind = EventKind::CrumbleShake, .at = m_map.cellCenter(cell), .cell = cell});
            }
        }
    };
    const int left = static_cast<int>(std::floor((box.left() + 0.01f) / kTile));
    const int right = static_cast<int>(std::floor((box.right() - 0.01f) / kTile));
    if (m_hero.grounded) {
        const int below = static_cast<int>(std::floor((box.bottom() + 0.5f) / kTile));
        for (int x = left; x <= right; ++x) {
            trigger({x, below});
        }
    }
    if (m_hero.wallSliding && m_hero.wallSide != 0) {
        const int column = static_cast<int>(
            std::floor((m_hero.wallSide > 0 ? box.right() + 0.5f : box.left() - 0.5f) / kTile));
        const int top = static_cast<int>(std::floor((box.top() + 0.01f) / kTile));
        const int bottom = static_cast<int>(std::floor((box.bottom() - 0.01f) / kTile));
        for (int y = top; y <= bottom; ++y) {
            trigger({column, y});
        }
    }
}

void Session::checkSprings()
{
    if (m_hero.velocity.y < 0.0f) {
        return; // rising through it: only a landing or a walk-in launches
    }
    for (Spring& s : m_springs) {
        if (gamelib::core::overlaps(_springRect(s.cell), m_hero.box)) {
            m_hero.velocity.y = -kSpringSpeed;
            m_hero.grounded = false;
            m_hero.coyote = 0;
            m_hero.jumpBuffer = 0;
            m_hero.rising = false; // releasing jump does not cut a spring's launch
            s.bounce = 0;
            m_events.push({.kind = EventKind::Spring, .at = m_map.cellBottom(s.cell), .cell = s.cell});
            return;
        }
    }
}

void Session::checkFlags()
{
    for (Flag& flag : m_flags) {
        if (gamelib::core::overlaps(gamelib::core::cellRect(flag.cell, kTile).expanded(2.0f), m_hero.box)) {
            const Vec2 feet = m_map.cellBottom(flag.cell);
            if (m_checkpoint.reach(m_room, feet)) {
                for (Flag& other : m_flags) {
                    other.reached = false;
                }
                flag.reached = true;
                m_events.push({.kind = EventKind::Checkpoint, .at = feet, .cell = flag.cell});
            }
        }
    }
}

bool Session::hitBySaw() const
{
    const Rect body = m_hero.box.expanded(-1.0f);
    for (const Saw& saw : m_saws) {
        if (gamelib::core::overlaps(saw.position, saw.radius * kSawHitShrink, body)) {
            return true;
        }
    }
    return false;
}

bool Session::exitAt(const Side side) const
{
    return m_exits[static_cast<std::size_t>(side)];
}

void Session::tick(const Input& input)
{
    m_previousBox = m_hero.box;
    if (m_phase == Phase::Exiting || m_phase == Phase::Finished) {
        return;
    }
    ++m_ticks;
    ++m_worldTicks;
    ++m_roomTicks;
    stepObjects();

    if (m_phase == Phase::Dying) {
        if (++m_dyingTicks >= kDeathTicks) {
            resetRoomObjects();
            spawnHero(m_checkpoint.position());
            m_phase = Phase::Playing;
            m_events.push({.kind = EventKind::Respawn, .at = m_checkpoint.position()});
        }
        return;
    }

    const gamelib::core::PlatformerIntent intent{
        .move = static_cast<float>(input.move),
        .jumpPressed = input.jumpPressed,
        .jumpHeld = input.jumpHeld || input.jumpPressed,
    };
    const float fallSpeed = m_hero.velocity.y;
    const gamelib::core::PlatformerStep step = gamelib::core::stepPlatformer(m_hero, intent, m_tuning, tileWorld());
    const Vec2 feet{m_hero.box.center().x, m_hero.box.bottom()};
    if (step.events.jumped) {
        m_events.push({.kind = EventKind::Jump, .at = feet});
    }
    if (step.events.wallJumped) {
        m_events.push({.kind = EventKind::WallJump, .at = m_hero.box.center(), .side = m_hero.facing > 0 ? Side::Left : Side::Right});
    }
    if (step.events.landed) {
        m_events.push({.kind = EventKind::Land, .at = feet, .strength = fallSpeed});
    }

    touchCrumbles();
    checkSprings();
    checkFlags();

    if (step.contacts.hazard || hitBySaw()) {
        die();
        return;
    }

    const Vec2 center = m_hero.box.center();
    std::optional<Side> left;
    if (center.x < 0.0f) {
        left = Side::Left;
    } else if (center.x > kRoomSize.x) {
        left = Side::Right;
    } else if (center.y < 0.0f) {
        left = Side::Up;
    } else if (m_hero.box.top() > kRoomSize.y || (!exitAt(Side::Down) && m_hero.box.bottom() > kRoomSize.y + 2.0f)) {
        left = Side::Down;
    }
    if (left) {
        if (*left == Side::Down && !exitAt(Side::Down)) {
            die(); // a pit
            return;
        }
        m_phase = Phase::Exiting;
        m_exitSide = *left;
        m_events.push({.kind = EventKind::RoomExit, .at = center, .side = *left});
        if ((m_room + 1) % kRoomsPerWorld == 0 && m_room < kSummitRoom) {
            m_events.push({.kind = EventKind::WorldClear, .at = center, .world = worldOf(m_room),
                           .ticks = m_worldTicks, .deaths = m_worldDeaths});
        }
        return;
    }

    if (m_hasGoal && gamelib::core::overlaps(Rect::fromCenter(m_goal, {kTile * 1.5f, kTile * 2.0f}), m_hero.box)) {
        m_phase = Phase::Finished;
        m_events.push({.kind = EventKind::Summit, .at = m_goal, .ticks = m_ticks, .deaths = m_deaths});
    }
}

std::uint64_t Session::hash() const
{
    gamelib::core::StateHash h;
    h.add(m_room).add(static_cast<int>(m_phase)).add(m_ticks).add(m_deaths);
    h.add(m_hero.box.position()).add(m_hero.velocity).add(m_hero.grounded).add(m_hero.coyote).add(m_hero.jumpBuffer);
    for (const Crumble& c : m_crumbles) {
        h.add(static_cast<int>(c.state)).add(c.timer);
    }
    for (const Saw& s : m_saws) {
        h.add(s.position);
    }
    return h.value();
}

} // namespace leap::game
