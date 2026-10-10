#include "view.hpp"

#include "palette.hpp"

#include "backdrop_frag.spv.hpp"
#include "flag_frag.spv.hpp"
#include "hero_frag.spv.hpp"
#include "saw_frag.spv.hpp"
#include "tile_frag.spv.hpp"

#include <gamelib/core/collision.hpp>
#include <gamelib/draw/effects.hpp>
#include <gamelib/draw/vector_text.hpp>
#include <gamelib/fx/sprite_motion.hpp>

#include <algorithm>
#include <cmath>
#include <vector>

namespace leap::app {

namespace {

// helpers

using gamelib::core::Cell;
using gamelib::core::Color;
using gamelib::core::Rect;
using gamelib::core::Vec2;
using game::kTile;

enum TileKind { kRock = 0, kSpikeUp = 1, kSpikeDown = 2, kSpikeLeft = 3, kSpikeRight = 4, kCrumble = 5, kSpringKind = 6 };

constexpr float kEdgeWidth = 1.1f;
constexpr float kCornerRadius = 3.5f;

std::uint64_t _tileBits(const Cell cell, const std::uint8_t mask)
{
    return static_cast<std::uint64_t>(mask) | (static_cast<std::uint64_t>(gamelib::draw::tileVariant(cell)) << 8)
           | (static_cast<std::uint64_t>(static_cast<std::uint16_t>(cell.x)) << 32)
           | (static_cast<std::uint64_t>(static_cast<std::uint16_t>(cell.y)) << 48);
}

gamelib::draw::TileSet _tileSet(const Shapes& shapes, const WorldPalette& palette)
{
    gamelib::draw::TileSet set;
    set.set('#', {.shape = shapes.tile, .color = palette.rock, .color2 = palette.edge,
                  .params = {static_cast<float>(kRock), kEdgeWidth, kCornerRadius, 0.0f}});
    const auto spike = [&](const char id, const TileKind kind, const std::uint8_t group) {
        set.set(gamelib::core::charTile(id), {.shape = shapes.tile, .color = kHazard, .color2 = kHazardGlow,
                                              .params = {static_cast<float>(kind), 0.0f, 0.0f, 0.0f}, .group = group});
    };
    spike('^', kSpikeUp, 1);
    spike('v', kSpikeDown, 2);
    spike('<', kSpikeLeft, 3);
    spike('>', kSpikeRight, 4);
    return set;
}

void _drawBackdrop(gamelib::draw::DrawList& list, const Shapes& shapes, const int world, const gamelib::core::Size canvas,
                   const float boost)
{
    const WorldPalette& palette = worldPalette(world);
    const Rect full{0.0f, 0.0f, static_cast<float>(canvas.width), static_cast<float>(canvas.height)};
    list.setLayer(gamelib::draw::layers::Background);
    list.vignette(full, palette.skyLow, 1.25f, 0.45f);
    list.add({.rect = full, .color = palette.haze, .color2 = palette.accent, .shape = shapes.backdrop,
              .params = {static_cast<float>(world), boost, 0.0f, 0.0f}});
}

void _drawCrumbles(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session,
                   const WorldPalette& palette, const float time)
{
    const gamelib::core::TileLayer& layer = session.map().layer(0);
    for (const game::Crumble& c : session.crumbles()) {
        Rect rect = gamelib::core::cellRect(c.cell, kTile);
        float cracks = 0.0f;
        Color fill = palette.rock.scaled(1.25f);
        Color edge = palette.edge.scaled(0.75f);
        if (c.state == game::Crumble::State::Shaking) {
            const float t = 1.0f - static_cast<float>(c.timer) / static_cast<float>(game::kCrumbleShakeTicks);
            cracks = t;
            rect.x += std::sin(time * 70.0f + static_cast<float>(c.cell.x)) * 0.6f * (0.3f + t);
            rect.y += std::cos(time * 55.0f + static_cast<float>(c.cell.y)) * 0.4f * (0.3f + t);
        } else if (c.state == game::Crumble::State::Gone) {
            // A faint ghost where it will come back, brighter just before it does.
            const float back = 1.0f - std::min(1.0f, static_cast<float>(c.timer) / 40.0f);
            fill = fill.withAlpha(0.06f + 0.25f * back);
            edge = edge.withAlpha(0.15f + 0.5f * back);
            cracks = 1.0f;
        }
        const std::uint8_t mask = gamelib::core::neighborMask(layer, c.cell, gamelib::core::Neighbors::Eight,
                                                              [](const gamelib::core::TileId id) { return id == 'C'; });
        list.add({.rect = rect, .color = fill, .color2 = edge, .shape = shapes.tile,
                  .params = {static_cast<float>(kCrumble), kEdgeWidth, kCornerRadius, cracks},
                  .bits = _tileBits(c.cell, c.state == game::Crumble::State::Gone ? 0 : mask)});
    }
}

void _drawSprings(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session)
{
    for (const game::Spring& s : session.springs()) {
        // Just fired: the pad shoots up, then settles back.
        const float t = std::min(1.0f, static_cast<float>(s.bounce) / 14.0f);
        const float squash = -0.7f * (1.0f - t) * (1.0f - t);
        list.add({.rect = gamelib::core::cellRect(s.cell, kTile), .color = kSpring, .color2 = kSpringGlow,
                  .shape = shapes.tile, .params = {static_cast<float>(kSpringKind), 0.0f, 0.0f, squash},
                  .bits = _tileBits(s.cell, 0)});
    }
}

void _drawFlags(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session)
{
    for (const game::Flag& flag : session.flags()) {
        const Rect cell = gamelib::core::cellRect(flag.cell, kTile);
        list.add({.rect = {cell.x - 2.0f, cell.y - kTile, kTile + 6.0f, 2.0f * kTile},
                  .color = flag.reached ? kFlagReached : kFlag,
                  .color2 = glow(flag.reached ? kFlagReached : kFlag, 1.5f), .shape = shapes.flag,
                  .params = {flag.reached ? 1.0f : 0.35f, static_cast<float>(flag.cell.x), 0.0f, 0.0f}});
    }
    if (session.hasGoal()) {
        const Vec2 g = session.goal();
        list.add({.rect = {g.x - 24.0f, g.y - 40.0f, 48.0f, 48.0f}, .color = kHero, .color2 = glow(kHero, 1.8f),
                  .shape = shapes.flag, .params = {0.0f, 0.0f, 1.0f, 0.0f}});
    }
}

void _drawSaws(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session,
               const WorldPalette& palette, const float alpha)
{
    const game::RoomDef& def = session.roomDef();
    // The tracks first: a dotted line along each path.
    for (const game::SawTrack& track : def.saws) {
        std::vector<Vec2> points;
        for (const Vec2 p : track.points) {
            points.push_back({(p.x + 0.5f) * kTile, (p.y + 0.5f) * kTile});
        }
        if (track.loop) {
            points.push_back(points.front());
        }
        for (std::size_t i = 0; i + 1 < points.size(); ++i) {
            const Vec2 a = points[i];
            const Vec2 b = points[i + 1];
            const float length = gamelib::core::length(b - a);
            const int dots = std::max(1, static_cast<int>(length / 5.0f));
            for (int d = 0; d <= dots; ++d) {
                list.circle(gamelib::core::lerp(a, b, static_cast<float>(d) / static_cast<float>(dots)), 0.7f,
                            palette.edge.scaled(0.35f).withAlpha(0.8f));
            }
        }
    }
    for (const game::Saw& saw : session.saws()) {
        const Vec2 at = gamelib::core::lerp(saw.previous, saw.position, alpha);
        const float margin = 5.0f;
        const float size = 2.0f * (saw.radius + margin);
        list.add({.rect = Rect::fromCenter(at, {size, size}), .color = kSaw, .color2 = kSawGlow, .shape = shapes.saw,
                  .rotation = saw.spin, .params = {saw.radius, 12.0f, margin, 0.0f}});
    }
}

void _drawHero(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session, const Effects& effects,
               const float alpha)
{
    const gamelib::core::PlatformerBody& hero = session.hero();
    const Vec2 position = gamelib::core::lerp(session.previousHeroBox().position(), hero.box.position(), alpha);
    const Rect box{position.x, position.y, hero.box.width, hero.box.height};
    // The drawing is a little larger than the hitbox (forgiving edges).
    const Rect drawn{box.x - 1.5f, box.y - 1.5f, box.width + 3.0f, box.height + 1.5f};
    Vec2 scale = effects.heroScale();
    if (hero.wallSliding) {
        scale = {scale.x * 0.92f, scale.y * 1.05f};
    }
    const Rect body = gamelib::fx::scaledFromBottom(drawn, scale);
    const float margin = 4.0f;
    const float look = std::clamp(hero.velocity.y / 5.0f, -1.0f, 1.0f);
    list.add({.rect = body.expanded(margin), .color = kHero, .color2 = kHeroEdge, .shape = shapes.hero,
              .params = {static_cast<float>(hero.facing), effects.blink(), look, margin}});
}

} // namespace

Shapes Shapes::registerWith(gamelib::gfx::Renderer& renderer)
{
    return {.tile = renderer.addShape(shaders::tile_frag),
            .hero = renderer.addShape(shaders::hero_frag),
            .saw = renderer.addShape(shaders::saw_frag),
            .backdrop = renderer.addShape(shaders::backdrop_frag),
            .flag = renderer.addShape(shaders::flag_frag)};
}

gamelib::core::View2D roomView(const gamelib::core::Size canvas, const Vec2 shake)
{
    const float zoom = static_cast<float>(canvas.width) / game::kRoomSize.x;
    const float visibleHeight = static_cast<float>(canvas.height) / zoom;
    return {.origin = Vec2{0.0f, (game::kRoomSize.y - visibleHeight) / 2.0f} - shake, .zoom = zoom};
}

int paletteWorld(const int room)
{
    return room >= game::kSummitRoom ? 4 : game::worldOf(room);
}

void drawRoom(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session, const Effects& effects,
              const gamelib::core::Size canvas, const Vec2 shake, const RoomDrawOptions& options)
{
    const int world = paletteWorld(session.room());
    const WorldPalette& palette = worldPalette(world);
    list.setView({});
    _drawBackdrop(list, shapes, world, canvas, options.backdropBoost);

    const gamelib::core::View2D view = roomView(canvas, shake);
    // The hint, in canvas units, behind the tiles.
    const game::RoomDef& def = session.roomDef();
    if (options.hintAlpha > 0.0f && !def.hint.empty()) {
        list.setLayer(gamelib::draw::layers::Parallax);
        const Vec2 at = view.toCanvas({(def.hintAt.x + 0.5f) * kTile, (def.hintAt.y + 0.5f) * kTile});
        gamelib::draw::drawVectorText(list, def.hint, at.x, at.y,
                                      {.size = 34.0f, .color = palette.accent.withAlpha(0.55f * options.hintAlpha), .weight = 0.9f,
                                       .align = gamelib::draw::Align::Center});
    }

    list.setView(view);
    list.setLayer(gamelib::draw::layers::TilesBehind);
    const gamelib::draw::TileSet tiles = _tileSet(shapes, palette);
    gamelib::draw::drawTiles(list, session.map(), 0, tiles, view.visibleWorld(canvas));
    _drawCrumbles(list, shapes, session, palette, options.time);
    _drawSprings(list, shapes, session);
    _drawFlags(list, shapes, session);

    list.setLayer(gamelib::draw::layers::Entities);
    _drawSaws(list, shapes, session, palette, options.alpha);
    if (options.showHero && session.phase() != game::Phase::Dying) {
        _drawHero(list, shapes, session, effects, options.alpha);
    }
    list.setLayer(gamelib::draw::layers::Effects);
    effects.drawWorld(list);
}

} // namespace leap::app
