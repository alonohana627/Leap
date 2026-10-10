#pragma once

// Presentation-only juice, on real frame time: dust puffs on jumps, landings
// and wall slides, the burst and flash of a death, rings for checkpoints and
// springs, the hero's squash and stretch and blinking, the screen shake.
// Everything is in world units (drawn under the room's view) except the flash.

#include "session.hpp"

#include <gamelib/core/color.hpp>
#include <gamelib/core/math.hpp>
#include <gamelib/draw/draw_list.hpp>
#include <gamelib/fx/juice.hpp>
#include <gamelib/fx/particles.hpp>
#include <gamelib/fx/sprite_motion.hpp>
#include <gamelib/fx/transients.hpp>

namespace leap::app {

class Effects {
public:
    Effects();

    // Reacts to one simulation event (world: the palette's world).
    void onEvent(const game::Event& event, int world);
    // While the hero slides down a wall: a little dust at the contact point.
    void wallSlide(gamelib::core::Vec2 contact, float seconds);
    // Fireworks over the summit (the ending).
    void firework(gamelib::core::Vec2 at, gamelib::core::Color color);
    void update(float seconds);
    void clear();

    void drawWorld(gamelib::draw::DrawList& list) const;
    // The death flash over the whole canvas (canvas units).
    void drawFlash(gamelib::draw::DrawList& list, gamelib::core::Size canvas) const;

    [[nodiscard]] gamelib::core::Vec2 shake(float time) const { return m_shake.offset(time); }
    [[nodiscard]] gamelib::core::Vec2 heroScale() const { return m_squash.scale(); }
    [[nodiscard]] float blink() const;

private:
    struct Puff {
        gamelib::core::Vec2 at;
        gamelib::core::Vec2 drift;
        float size = 4.0f;
        gamelib::core::Color color;
    };
    struct Ring {
        gamelib::core::Vec2 at;
        float radius = 20.0f;
        gamelib::core::Color color;
    };

    void puffs(gamelib::core::Vec2 at, int count, float spread, gamelib::core::Vec2 drift, gamelib::core::Color color);

    gamelib::fx::Particles m_debris;
    gamelib::fx::Transients<Puff> m_puffs;
    gamelib::fx::Transients<Ring> m_rings;
    gamelib::fx::Fade m_flash{0.22f};
    gamelib::fx::Shake m_shake{0.3f, 3.0f}; // world units: about 11 canvas units at the room's zoom
    gamelib::fx::SquashStretch m_squash{7.0f, 0.4f};
    gamelib::core::Rng m_rng;
    float m_time = 0.0f;
    float m_slideDust = 0.0f;
};

} // namespace leap::app
