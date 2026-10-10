#include "effects.hpp"

#include "palette.hpp"

#include <gamelib/core/easing.hpp>

#include <algorithm>
#include <cmath>
#include <numbers>

namespace leap::app {

namespace {

// helpers

using gamelib::core::Color;
using gamelib::core::Vec2;

constexpr float kPi = std::numbers::pi_v<float>;
const Color kDust{0.85f, 0.88f, 0.95f, 0.55f};

} // namespace

Effects::Effects()
    : m_debris{400, {0.0f, 260.0f}, 0.25f}
    , m_rng{gamelib::core::randomSeed()}
{
}

void Effects::puffs(const Vec2 at, const int count, const float spread, const Vec2 drift, const Color color)
{
    for (int i = 0; i < count; ++i) {
        const float offset = (static_cast<float>(i) - static_cast<float>(count - 1) / 2.0f) * spread;
        m_puffs.add({.at = at + Vec2{offset, -1.0f},
                     .drift = drift + Vec2{offset * 1.5f + m_rng.uniform(-4.0f, 4.0f), m_rng.uniform(-10.0f, -3.0f)},
                     .size = m_rng.uniform(2.5f, 4.0f),
                     .color = color},
                    m_rng.uniform(0.28f, 0.42f));
    }
}

void Effects::onEvent(const game::Event& event, const int world)
{
    using game::EventKind;
    const WorldPalette& palette = worldPalette(world);
    switch (event.kind) {
    case EventKind::Jump:
        puffs(event.at, 3, 3.0f, {}, kDust);
        m_squash.kick(0.35f);
        break;
    case EventKind::WallJump: {
        const float away = event.side == game::Side::Left ? -1.0f : 1.0f; // the wall's side
        puffs(event.at + Vec2{away * 5.0f, 4.0f}, 3, 2.5f, {-away * 12.0f, 0.0f}, kDust);
        m_squash.kick(0.3f);
        break;
    }
    case EventKind::Land: {
        const float strength = std::clamp(event.strength / 6.5f, 0.0f, 1.0f);
        puffs(event.at, 2 + static_cast<int>(strength * 3.0f), 3.5f, {}, kDust);
        m_squash.kick(-0.12f - 0.35f * strength);
        break;
    }
    case EventKind::Spring:
        m_rings.add({.at = event.at, .radius = 14.0f, .color = kSpringGlow}, 0.35f);
        puffs(event.at, 4, 3.0f, {}, kDust);
        m_squash.kick(0.6f);
        break;
    case EventKind::CrumbleBreak:
        m_debris.burst(event.at, 8,
                       {.direction = {-kPi * 0.9f, -kPi * 0.1f}, .speed = {20.0f, 70.0f}, .size = {2.0f, 4.0f},
                        .life = {0.5f, 0.9f}, .spin = {-6.0f, 6.0f}, .color = palette.edge.scaled(0.6f)});
        break;
    case EventKind::CrumbleReturn:
        m_rings.add({.at = event.at, .radius = 10.0f, .color = palette.edge.scaled(0.7f)}, 0.3f);
        break;
    case EventKind::Death:
        m_debris.burst(event.at, 22,
                       {.speed = {40.0f, 160.0f}, .size = {2.0f, 4.5f}, .life = {0.35f, 0.7f}, .spin = {-10.0f, 10.0f},
                        .color = kHero});
        m_debris.burst(event.at, 10,
                       {.speed = {60.0f, 200.0f}, .size = {1.5f, 2.5f}, .life = {0.2f, 0.4f}, .color = kHazardGlow});
        m_rings.add({.at = event.at, .radius = 28.0f, .color = kHeroEdge}, 0.35f);
        m_flash.trigger(1.0f);
        m_shake.trigger(1.0f);
        m_squash.reset();
        break;
    case EventKind::Respawn:
        m_rings.add({.at = event.at + Vec2{0.0f, -7.0f}, .radius = 16.0f, .color = kHeroEdge}, 0.3f);
        m_squash.reset();
        m_squash.kick(-0.4f);
        break;
    case EventKind::Checkpoint:
        m_rings.add({.at = event.at + Vec2{0.0f, -8.0f}, .radius = 30.0f, .color = glow(kFlagReached, 1.5f)}, 0.5f);
        m_debris.burst(event.at + Vec2{0.0f, -12.0f}, 12,
                       {.direction = {-kPi, 0.0f}, .speed = {30.0f, 90.0f}, .size = {1.5f, 2.5f}, .life = {0.4f, 0.8f},
                        .color = glow(kFlagReached, 1.3f)});
        break;
    case EventKind::Summit:
        m_rings.add({.at = event.at, .radius = 60.0f, .color = glow(kHero, 1.6f)}, 0.8f);
        m_flash.trigger(0.6f);
        break;
    case EventKind::CrumbleShake:
    case EventKind::RoomExit:
    case EventKind::WorldClear: break;
    }
}

void Effects::wallSlide(const Vec2 contact, const float seconds)
{
    m_slideDust += seconds;
    if (m_slideDust >= 0.07f) {
        m_slideDust = 0.0f;
        m_puffs.add({.at = contact, .drift = {m_rng.uniform(-6.0f, 6.0f), m_rng.uniform(-14.0f, -6.0f)},
                     .size = m_rng.uniform(1.5f, 2.5f), .color = kDust},
                    0.3f);
    }
}

void Effects::firework(const Vec2 at, const Color color)
{
    m_debris.burst(at, 26, {.speed = {40.0f, 120.0f}, .size = {1.5f, 2.5f}, .life = {0.6f, 1.1f}, .color = color});
    m_rings.add({.at = at, .radius = 26.0f, .color = color}, 0.5f);
}

void Effects::update(const float seconds)
{
    m_time += seconds;
    m_debris.update(seconds);
    m_puffs.update(seconds);
    m_rings.update(seconds);
    m_flash.update(seconds);
    m_shake.update(seconds);
    m_squash.update(seconds);
}

void Effects::clear()
{
    m_debris.clear();
    m_puffs.clear();
    m_rings.clear();
    m_flash.clear();
    m_shake.clear();
    m_squash.reset();
}

float Effects::blink() const
{
    // A blink of 0.14 s every 3.1 s.
    const float phase = std::fmod(m_time, 3.1f);
    return phase < 0.14f ? std::sin(phase / 0.14f * kPi) : 0.0f;
}

void Effects::drawWorld(gamelib::draw::DrawList& list) const
{
    for (const auto& puff : m_puffs) {
        const float t = puff.progress();
        const Vec2 at = puff.value.at + puff.value.drift * puff.age;
        const float size = puff.value.size * (0.6f + 0.8f * gamelib::core::easeOutCubic(t));
        list.circle(at, size, puff.value.color.withAlpha(puff.value.color.a * (1.0f - t)));
    }
    for (const auto& ring : m_rings) {
        const float t = ring.progress();
        const float radius = ring.value.radius * (0.3f + 0.7f * gamelib::core::easeOutCubic(t));
        list.circle(ring.value.at, radius, ring.value.color.withAlpha(1.0f - t), {.ringWidth = 1.2f + 1.5f * (1.0f - t)});
    }
    m_debris.draw(list, {}, {.aspect = 1.0f, .cornerRadius = 0.6f, .endScale = 0.2f});
}

void Effects::drawFlash(gamelib::draw::DrawList& list, const gamelib::core::Size canvas) const
{
    if (m_flash.active()) {
        const float a = m_flash.value();
        list.box({0.0f, 0.0f, static_cast<float>(canvas.width), static_cast<float>(canvas.height)},
                 Color{1.0f, 0.95f, 0.9f, 0.16f * a * a});
    }
}

} // namespace leap::app
