#pragma once

// Drawing a room and what is in it: the world's backdrop, the tiles (one
// tile shader for rock, spikes, crumbling blocks and springs), flags, the
// beacon, saws, the hint line and the hero, under a view that fits the
// 32 x 18 tile room to the canvas. Presentation only.

#include "effects.hpp"
#include "session.hpp"

#include <gamelib/core/camera.hpp>
#include <gamelib/draw/draw_list.hpp>
#include <gamelib/draw/tile_map.hpp>
#include <gamelib/gfx/renderer.hpp>

namespace leap::app {

// The game's own shader shapes, registered once.
struct Shapes {
    gamelib::draw::ShapeId tile;
    gamelib::draw::ShapeId hero;
    gamelib::draw::ShapeId saw;
    gamelib::draw::ShapeId backdrop;
    gamelib::draw::ShapeId flag;

    [[nodiscard]] static Shapes registerWith(gamelib::gfx::Renderer& renderer);
};

// The room's view: the whole room on the whole canvas.
[[nodiscard]] gamelib::core::View2D roomView(gamelib::core::Size canvas, gamelib::core::Vec2 shake = {});

struct RoomDrawOptions {
    float alpha = 0.0f;      // interpolation between the last two ticks
    float time = 0.0f;       // seconds, for animation
    float backdropBoost = 0.0f; // brighter backdrop (the title)
    float hintAlpha = 1.0f; // the teaching line fades out under banners
    bool showHero = true;
};

// Draws the session's room into the list (layers Background..Effects), with
// the effects; leaves the list's view at the room's view.
void drawRoom(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session, const Effects& effects,
              gamelib::core::Size canvas, gamelib::core::Vec2 shake, const RoomDrawOptions& options);

// The palette world of a room (the summit room has its own palette).
[[nodiscard]] int paletteWorld(int room);

} // namespace leap::app
