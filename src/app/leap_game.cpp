#include "leap_game.hpp"

#include "demo_run.hpp"
#include "palette.hpp"

#include <gamelib/app/settings.hpp>
#include <gamelib/core/easing.hpp>
#include <gamelib/draw/vector_text.hpp>
#include <gamelib/storage/files.hpp>
#include <gamelib/ui/menu.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <utility>

namespace leap::app {

namespace {

// helpers

using gamelib::app::ArcadeAction;
using gamelib::app::ArcadeScreen;
using gamelib::core::Color;
using gamelib::core::Vec2;
using gamelib::platform::GamepadButton;
using gamelib::platform::Key;

constexpr float kEndingMenuDelay = 5.0f;

std::shared_ptr<const Recording> _builtinDemo()
{
    std::optional<Recording> recording = Recording::parse(game::builtinDemo(), game::decodeInput);
    if (!recording) {
        recording.emplace(0, game::kRulesVersion); // never: the text is checked when it is made
    }
    return std::make_shared<const Recording>(std::move(*recording));
}

gamelib::ui::MenuStyle _menuStyle()
{
    return {.veil = Color{0.0f, 0.0f, 0.02f, 0.5f},
            .panel = srgb(0x0A0F1E, 0.94f),
            .border = srgb(0x3DF5C4),
            .title = kHero,
            .text = kText,
            .highlight = kHero,
            .selection = srgb(0x1B2A44),
            .dim = kTextDim,
            .titlePixel = 13.0f,
            .itemPixel = 6.0f,
            .glowWidth = 26.0f};
}

int _startRoomFromEnvironment()
{
    if (const char* text = std::getenv("LEAP_START_ROOM")) {
        const int room = std::atoi(text);
        if (room >= 0 && room < game::kRoomCount) {
            return room;
        }
    }
    return -1;
}

} // namespace

LeapGame::LeapGame(gamelib::app::App& app, ProfileFile profile, const bool demo)
    : m_profile{std::move(profile)}
    , m_menus{"LEAP", m_profile.value().settings,
              [this](gamelib::ui::Menu& title) {
                  title.setting(
                      "WORLD",
                      [this] {
                          const int w = m_profile.value().game.world;
                          return std::to_string(w + 1) + " " + std::string{game::worldName(w)};
                      },
                      [this](const int step) {
                          const int open = unlockedWorlds(m_profile.value().game.campaign);
                          int& w = m_profile.value().game.world;
                          w = ((w + step) % open + open) % open;
                      });
              },
              {.music = true}}
    , m_sfx{app.audio(), renderSfx}
    , m_music{app.audio()}
    , m_shapes{Shapes::registerWith(app.renderer())}
    , m_startRoom{_startRoomFromEnvironment()}
    , m_demoRecording{_builtinDemo()}
    , m_attract{[this](std::uint64_t) { return DemoRun{game::Session{0}, m_demoRecording, 0}; },
                [](DemoRun& run, float) {
                    if (run.session.phase() == game::Phase::Exiting || run.next >= run.recording->size()) {
                        return; // waiting for the wipe, or done
                    }
                    run.session.tick(run.recording->at(run.next++));
                },
                [](const DemoRun& run) {
                    return run.session.world() > 0
                           || (run.next >= run.recording->size() && run.session.phase() != game::Phase::Exiting);
                },
                1}
    , m_demoMode{demo}
{
    if (std::optional<Recording> own = loadDemo()) {
        m_demoRecording = std::make_shared<const Recording>(std::move(*own));
        m_attract.restart();
    }
    m_profile.save(); // a missing file is written with the defaults, so it can be found and edited
    gamelib::app::applySettings(app, m_profile.value().settings);
    m_music.play(app.audio(), 4);
}

void LeapGame::onFocus(gamelib::app::App& app, const bool focused)
{
    // A precision game must not run on while the player is away: pause.
    if (!focused && m_menus.is(ArcadeScreen::Playing) && !m_demoMode) {
        m_menus.go(ArcadeScreen::Paused, m_time);
        app.input().releaseAll();
    }
}

void LeapGame::onKey(gamelib::app::App& app, const gamelib::platform::KeyEvent& event)
{
    if (event.pressed && event.key == Key::F3) {
        m_debug.toggle();
        return;
    }
    if (m_demoMode) {
        if (event.pressed && event.key != Key::F11) {
            m_demoMode = false; // any key leaves the full-screen demo for the title
            app.input().releaseAll();
        }
        return;
    }
    handleResult(app, m_menus.onKey(app, event, m_time));
}

void LeapGame::onGamepad(gamelib::app::App& app, const gamelib::platform::GamepadEvent& event)
{
    if (m_demoMode) {
        if (event.pressed) {
            m_demoMode = false;
            app.input().releaseAll();
        }
        return;
    }
    handleResult(app, m_menus.onGamepad(app, event, m_time));
}

void LeapGame::handleResult(gamelib::app::App& app, const gamelib::app::ArcadeResult& result)
{
    if (result.changed) {
        m_profile.save();
    }
    switch (result.action) {
    case ArcadeAction::Start: startRun(app); break;
    case ArcadeAction::ToTitle: leaveRun(app); break;
    case ArcadeAction::Quit:
        m_profile.save();
        app.requestQuit();
        break;
    case ArcadeAction::Resume:
    case ArcadeAction::Controls:
    case ArcadeAction::None: break;
    }
    switch (result.feedback) {
    case gamelib::ui::MenuFeedback::Selected: m_sfx.queue(app.sounds(), Sfx::MenuSelect); break;
    case gamelib::ui::MenuFeedback::Back: m_sfx.queue(app.sounds(), Sfx::MenuBack); break;
    case gamelib::ui::MenuFeedback::Moved:
    case gamelib::ui::MenuFeedback::Changed: m_sfx.queue(app.sounds(), Sfx::MenuMove); break;
    case gamelib::ui::MenuFeedback::None: break;
    }
}

void LeapGame::startRun(gamelib::app::App& app)
{
    const int room = m_startRoom >= 0 ? m_startRoom : game::firstRoomOf(m_profile.value().game.world);
    m_session.emplace(room);
    m_recording = Recording{0, game::kRulesVersion};
    m_recordingWorldOne = room == 0;
    // A developer's check of the campaign end to end: LEAP_REPLAY names a
    // recording (core::InputRecording text) that drives the run instead of the keys.
    m_replay.reset();
    m_replayNext = 0;
    if (const char* path = std::getenv("LEAP_REPLAY")) {
        if (const std::optional<std::string> text = gamelib::storage::readFile(path)) {
            m_replay = Recording::parse(*text, game::decodeInput);
        }
    }
    m_effects.clear();
    m_wipe.cancel();
    m_banners.clear();
    m_ending = false;
    m_newBestRun = false;
    m_profile.save();
    announceWorld(room);
    m_music.play(app.audio(), paletteWorld(room));
}

void LeapGame::leaveRun(gamelib::app::App& app)
{
    m_session.reset();
    m_ending = false;
    m_wipe.cancel();
    m_banners.clear();
    m_profile.save();
    m_sfx.stopLoop(app.audio(), Sfx::SlideHiss);
    m_music.play(app.audio(), 4);
}

game::Input LeapGame::readInput(const gamelib::platform::InputState& input)
{
    game::Input in;
    const bool left = input.isAnyDown({Key::Left, Key::A}) || input.isDown(GamepadButton::DpadLeft)
                      || input.isDown(GamepadButton::LeftStickLeft);
    const bool right = input.isAnyDown({Key::Right, Key::D}) || input.isDown(GamepadButton::DpadRight)
                       || input.isDown(GamepadButton::LeftStickRight);
    in.move = (right ? 1 : 0) - (left ? 1 : 0);
    const bool held = input.isAnyDown({Key::Space, Key::Z, Key::C, Key::K, Key::Up, Key::W})
                      || input.isDown(GamepadButton::A) || input.isDown(GamepadButton::B);
    // Several ticks may run in one frame and all see the same transitions:
    // the press belongs to the first of them only.
    const bool pressed = !m_pressTaken
                         && (input.wasAnyPressed({Key::Space, Key::Z, Key::C, Key::K, Key::Up, Key::W})
                             || input.wasPressed(GamepadButton::A) || input.wasPressed(GamepadButton::B));
    m_pressTaken = true;
    in.jumpPressed = pressed;
    in.jumpHeld = held || pressed;
    return in;
}

void LeapGame::tick(gamelib::app::App& app)
{
    m_held = false;
    if (m_menus.is(ArcadeScreen::Title) || m_demoMode) {
        demoTick();
    }
    if (m_menus.is(ArcadeScreen::Playing) && !m_demoMode) {
        playTick(app);
    }
}

void LeapGame::playTick(gamelib::app::App& app)
{
    if (!m_session || m_wipe.active() || m_ending) {
        return;
    }
    const game::Input input = m_replay && m_replayNext < m_replay->size() ? m_replay->at(m_replayNext++)
                                                                          : readInput(app.input());
    if (m_recordingWorldOne) {
        m_recording.record(input);
    }
    m_session->tick(input);
    for (const game::Event& event : m_session->events().take()) {
        react(app, event);
    }
}

void LeapGame::demoTick()
{
    if (m_demoWipe.active()) {
        return;
    }
    m_attract.tick(1.0f / static_cast<float>(game::kTickRate));
    DemoRun& run = m_attract.sim();
    for (const game::Event& event : run.session.events().take()) {
        m_demoEffects.onEvent(event, paletteWorld(run.session.room()));
        if (event.kind == game::EventKind::RoomExit) {
            startWipe(m_demoWipe, event.side, [this] { m_attract.sim().session.enterNextRoom(); });
        }
    }
}

void LeapGame::startWipe(gamelib::ui::Transition& transition, const game::Side side, std::function<void()> atSwitch)
{
    using gamelib::ui::TransitionKind;
    TransitionKind kind = TransitionKind::WipeRight;
    switch (side) {
    case game::Side::Left: kind = TransitionKind::WipeLeft; break;
    case game::Side::Right: kind = TransitionKind::WipeRight; break;
    case game::Side::Up: kind = TransitionKind::WipeUp; break;
    case game::Side::Down: kind = TransitionKind::WipeDown; break;
    }
    transition.start({.kind = kind, .color = kInk, .coverSeconds = 0.16f, .holdSeconds = 0.04f, .uncoverSeconds = 0.18f},
                     std::move(atSwitch));
}

void LeapGame::announceWorld(const int room)
{
    if (room % game::kRoomsPerWorld != 0 && room != game::kSummitRoom) {
        return;
    }
    const int world = paletteWorld(room);
    const std::string title = room == game::kSummitRoom ? std::string{"THE SUMMIT"}
                                                        : "WORLD " + std::to_string(world + 1);
    m_banners.add({.title = title, .line = room == game::kSummitRoom ? "" : std::string{game::worldName(world)},
                   .color = worldPalette(world).accent},
                  2.6f);
}

void LeapGame::react(gamelib::app::App& app, const game::Event& event)
{
    using game::EventKind;
    const int world = paletteWorld(m_session->room());
    m_effects.onEvent(event, world);
    auto& sounds = app.sounds();
    const float pan = std::clamp(event.at.x / game::kRoomSize.x * 2.0f - 1.0f, -1.0f, 1.0f) * 0.5f;
    switch (event.kind) {
    case EventKind::Jump: m_sfx.queue(sounds, Sfx::Jump, 1.0f, 1.0f, pan); break;
    case EventKind::WallJump: m_sfx.queue(sounds, Sfx::WallJump, 1.0f, 1.0f, pan); break;
    case EventKind::Land:
        m_sfx.queue(sounds, Sfx::Land, std::clamp(event.strength / 5.0f, 0.3f, 1.0f), 1.0f, pan);
        break;
    case EventKind::Spring: m_sfx.queue(sounds, Sfx::Spring, 1.0f, 1.0f, pan); break;
    case EventKind::CrumbleShake: m_sfx.queue(sounds, Sfx::CrumbleShake, 0.8f, 1.0f, pan); break;
    case EventKind::CrumbleBreak: m_sfx.queue(sounds, Sfx::CrumbleBreak, 0.8f, 1.0f, pan); break;
    case EventKind::CrumbleReturn: break;
    case EventKind::Death:
        m_sfx.queue(sounds, Sfx::Death, 1.0f, 1.0f, pan);
        app.hitStop(0.05f);
        break;
    case EventKind::Respawn: m_sfx.queue(sounds, Sfx::Respawn, 1.0f, 1.0f, pan); break;
    case EventKind::Checkpoint: m_sfx.queue(sounds, Sfx::Checkpoint, 1.0f, 1.0f, pan); break;
    case EventKind::RoomExit:
        m_sfx.queue(sounds, Sfx::RoomExit);
        startWipe(m_wipe, event.side, [this, &app] {
            m_session->enterNextRoom();
            m_effects.clear();
            announceWorld(m_session->room());
            m_music.play(app.audio(), paletteWorld(m_session->room()));
        });
        break;
    case EventKind::WorldClear: {
        Progress& progress = m_profile.value().game;
        const gamelib::core::ClearResult result =
            progress.campaign.clear(worldLevel(event.world), {.ticks = event.ticks, .deaths = event.deaths});
        if (event.world + 1 < game::kWorlds && !result.unlocked.empty()) {
            progress.world = std::max(progress.world, event.world + 1); // the title offers the new world first
        }
        std::string line = "TIME " + formatTime(event.ticks) + "   DEATHS " + std::to_string(event.deaths);
        if (result.bestTime && !result.firstClear) {
            line += "   NEW BEST TIME";
        } else if (result.fewestDeaths && !result.firstClear) {
            line += "   FEWEST DEATHS";
        }
        m_banners.add({.title = "WORLD " + std::to_string(event.world + 1) + " CLEAR", .line = line,
                       .color = kHero},
                      3.2f);
        m_sfx.queue(sounds, Sfx::WorldClear);
        // The fastest run through world 1 becomes the title screen's demo.
        if (event.world == 0 && m_recordingWorldOne && result.bestTime) {
            saveDemo(m_recording);
            m_demoRecording = std::make_shared<const Recording>(m_recording);
        }
        m_recordingWorldOne = false;
        m_profile.save();
        break;
    }
    case EventKind::Summit: {
        m_ending = true;
        m_endingTime = 0.0f;
        m_nextFirework = 0.6f;
        m_sfx.queue(sounds, Sfx::Summit);
        m_sfx.stopLoop(app.audio(), Sfx::SlideHiss);
        if (m_session->startRoom() == 0) { // a full run from world 1 counts for the run's bests
            auto& bests = m_profile.value().bests;
            const bool time = bests.offer(kRunTime, event.ticks, gamelib::app::Bests::Better::Lower);
            const bool deaths = bests.offer(kRunDeaths, std::max(event.deaths, 0) + 1,
                                            gamelib::app::Bests::Better::Lower); // stored +1: 0 deaths is a value
            m_newBestRun = time || deaths;
        }
        m_profile.save();
        m_menus.gameOverAfter(kEndingMenuDelay, m_time);
        break;
    }
    }
}

void LeapGame::frame(gamelib::app::App& app, const gamelib::app::FrameInfo& info)
{
    m_time = info.time;
    m_pressTaken = false;
    m_held = info.ticks == 0;
    m_frameTicks = info.ticks;
    m_focusPaused = app.isFocusPaused();
    m_wipe.update(info.seconds);
    m_demoWipe.update(info.seconds);
    m_effects.update(info.seconds);
    m_demoEffects.update(info.seconds);
    m_banners.update(info.seconds);
    m_hintFade = gamelib::core::approach(m_hintFade, m_banners.empty() ? 1.0f : 0.0f, info.seconds * 2.5f);
    m_debug.frameTime(info.seconds);

    if (m_ending) {
        m_endingTime += info.seconds;
        m_nextFirework -= info.seconds;
        if (m_nextFirework <= 0.0f && m_session) {
            m_nextFirework = 0.45f;
            static constexpr std::array<std::uint32_t, 4> kColors{0xFFE45C, 0x3DF5C4, 0xFF5AB8, 0x63B8FF};
            const int i = static_cast<int>(m_endingTime * 7.0f) % 4;
            const Vec2 at = m_session->goal()
                            + Vec2{std::sin(m_endingTime * 3.1f) * 150.0f, -20.0f - std::abs(std::cos(m_endingTime * 2.3f)) * 60.0f};
            m_effects.firework(at, glow(srgb(kColors[static_cast<std::size_t>(i)]), 1.6f));
        }
    }
    // A run started while the window is in the background (a gamepad's Start
    // reaches the game without the focus) would sit frozen with no menu:
    // show the pause menu, as a focus loss during play does.
    if (app.isFocusPaused() && m_menus.is(ArcadeScreen::Playing) && !m_demoMode) {
        m_menus.go(ArcadeScreen::Paused, m_time);
        app.input().releaseAll();
    }
    if (m_menus.gameOverDue(m_time)) {
        m_menus.setGameOverTitle("THE SUMMIT");
        m_menus.go(ArcadeScreen::GameOver, m_time);
        app.input().releaseAll();
    }
    updateAudio(app);
}

void LeapGame::updateAudio(gamelib::app::App& app)
{
    // The wall-slide hiss loops while the hero slides, and stops on any menu.
    const bool sliding = m_session && m_menus.is(ArcadeScreen::Playing) && !m_demoMode && !m_wipe.active()
                         && m_session->phase() == game::Phase::Playing && m_session->hero().wallSliding;
    if (sliding) {
        m_sfx.loop(app.audio(), Sfx::SlideHiss, 0.8f);
        const auto& hero = m_session->hero();
        const Vec2 contact{hero.wallSide > 0 ? hero.box.right() : hero.box.left(), hero.box.top() + 3.0f};
        m_effects.wallSlide(contact, 1.0f / 60.0f);
    } else {
        m_sfx.stopLoop(app.audio(), Sfx::SlideHiss);
    }
    if (m_menus.is(ArcadeScreen::Title) || m_demoMode) {
        m_music.play(app.audio(), 4);
    }
}

bool LeapGame::showsRun() const
{
    return m_session && !m_demoMode && !m_menus.is(ArcadeScreen::Title);
}

void LeapGame::draw(gamelib::app::App& app, gamelib::draw::DrawList& list, const gamelib::app::FrameInfo& info)
{
    const gamelib::core::Size canvas = app.canvas();
    const bool shakeOn = m_profile.value().settings.screenShake;
    // Bloom makes the neon edges glow; the HUD and menus stay crisp on top.
    list.setPostProcess({.bloom = {.intensity = 0.55f, .threshold = 0.85f, .radius = 1.2f}});

    if (showsRun()) {
        const bool frozen = m_held || !m_menus.is(ArcadeScreen::Playing) || m_wipe.active();
        const Vec2 shake = shakeOn ? m_effects.shake(info.time) : Vec2{};
        drawRoom(list, m_shapes, *m_session, m_effects, canvas, shake,
                 {.alpha = frozen ? 1.0f : info.alpha, .time = info.time, .hintAlpha = m_ending ? 0.0f : m_hintFade});
        list.setView({});
        list.setLayer(gamelib::draw::layers::Hud);
        m_effects.drawFlash(list, canvas);
        drawHud(list, canvas, *m_session);
        m_wipe.draw(list, canvas);
        drawBanners(list, canvas);
        if (m_ending) {
            drawEnding(list, canvas);
        }
        drawDebug(list, canvas, *m_session);
    } else {
        const DemoRun& run = m_attract.sim();
        drawRoom(list, m_shapes, run.session, m_demoEffects, canvas, {},
                 {.alpha = m_demoWipe.active() ? 1.0f : info.alpha, .time = info.time, .backdropBoost = 0.3f,
                  .hintAlpha = m_demoMode ? 1.0f : 0.0f});
        list.setView({});
        list.setLayer(gamelib::draw::layers::Hud);
        m_demoWipe.draw(list, canvas);
        if (m_demoMode) {
            drawHud(list, canvas, run.session);
            list.text("DEMO", static_cast<float>(canvas.width) / 2.0f, static_cast<float>(canvas.height) - 70.0f,
                      {.pixelSize = 5.0f, .color = kText.withAlpha(0.6f + 0.4f * std::sin(info.time * 3.0f)),
                       .align = gamelib::draw::Align::Center, .shadow = true});
        }
        drawDebug(list, canvas, run.session);
    }
    if (!m_demoMode) {
        drawMenus(list, canvas);
    }
}

void LeapGame::drawHud(gamelib::draw::DrawList& list, const gamelib::core::Size canvas,
                       const game::Session& session) const
{
    const int room = session.room();
    const std::string label = room == game::kSummitRoom
                                  ? std::string{"FINAL"}
                                  : std::to_string(game::worldOf(room) + 1) + "-" + std::to_string(room % 8 + 1);
    const Color accent = worldPalette(paletteWorld(room)).accent;
    // Dark plates under the text, so a gap in the top border (an exit) never cuts it.
    const Color plate{0.02f, 0.03f, 0.07f, 0.85f};
    const float leftWidth = gamelib::draw::textWidth(label, 4.0f) + 28.0f
                            + gamelib::draw::textWidth(session.roomDef().name, 4.0f);
    list.box({20.0f, 8.0f, leftWidth + 28.0f, 48.0f}, plate, {.cornerRadius = 10.0f});
    list.box({static_cast<float>(canvas.width) - 300.0f - 34.0f - 14.0f, 8.0f, 328.0f, 48.0f}, plate,
             {.cornerRadius = 10.0f});
    list.text(label, 34.0f, 18.0f, {.pixelSize = 4.0f, .color = accent, .shadow = true});
    list.text(std::string{session.roomDef().name}, 34.0f + gamelib::draw::textWidth(label, 4.0f) + 28.0f, 18.0f,
              {.pixelSize = 4.0f, .color = kText.withAlpha(0.85f), .shadow = true});
    const float right = static_cast<float>(canvas.width) - 34.0f;
    list.text(formatTime(session.ticks()), right, 18.0f,
              {.pixelSize = 4.0f, .color = kText, .align = gamelib::draw::Align::Right, .shadow = true});
    list.text("DEATHS " + std::to_string(session.deaths()), right - 230.0f, 18.0f,
              {.pixelSize = 4.0f, .color = kText.withAlpha(0.85f), .align = gamelib::draw::Align::Right,
               .shadow = true});
}

void LeapGame::drawBanners(gamelib::draw::DrawList& list, const gamelib::core::Size canvas) const
{
    float y = static_cast<float>(canvas.height) * 0.3f;
    for (const auto& banner : m_banners) {
        const float t = banner.progress();
        const float in = gamelib::core::easeOutCubic(std::min(1.0f, banner.age / 0.3f));
        const float out = 1.0f - gamelib::core::smoothstep(0.75f, 1.0f, t);
        const float alpha = in * out;
        const float cx = static_cast<float>(canvas.width) / 2.0f;
        const float width = 900.0f;
        list.box({cx - width / 2.0f, y - 70.0f, width, banner.value.line.empty() ? 130.0f : 190.0f},
                 Color{0.02f, 0.03f, 0.07f, 0.72f * alpha},
                 {.cornerRadius = 18.0f, .borderWidth = 3.0f, .borderColor = banner.value.color.withAlpha(0.8f * alpha)});
        gamelib::draw::drawVectorText(list, banner.value.title, cx, y - 46.0f - (1.0f - in) * 20.0f,
                                      {.size = 74.0f, .color = banner.value.color.withAlpha(alpha), .weight = 1.4f,
                                       .align = gamelib::draw::Align::Center});
        if (!banner.value.line.empty()) {
            list.text(banner.value.line, cx, y + 62.0f,
                      {.pixelSize = 5.0f, .color = kText.withAlpha(alpha), .align = gamelib::draw::Align::Center});
        }
        y += 240.0f;
    }
}

void LeapGame::drawEnding(gamelib::draw::DrawList& list, const gamelib::core::Size canvas) const
{
    if (!m_session || m_menus.is(ArcadeScreen::GameOver)) {
        return;
    }
    const float a = gamelib::core::smoothstep(0.6f, 1.6f, m_endingTime);
    const float cx = static_cast<float>(canvas.width) / 2.0f;
    list.box({cx - 560.0f, 480.0f, 1120.0f, m_newBestRun ? 330.0f : 270.0f}, Color{0.02f, 0.02f, 0.08f, 0.75f * a},
             {.cornerRadius = 24.0f, .borderWidth = 3.0f, .borderColor = kHero.withAlpha(0.7f * a)});
    gamelib::draw::drawVectorText(list, "You reached the summit.", cx, 560.0f,
                                  {.size = 64.0f, .color = kHero.withAlpha(a), .weight = 1.3f,
                                   .align = gamelib::draw::Align::Center});
    const float b = gamelib::core::smoothstep(1.6f, 2.6f, m_endingTime);
    list.text("TIME " + formatTime(m_session->ticks()) + "     DEATHS " + std::to_string(m_session->deaths()), cx,
              680.0f, {.pixelSize = 6.0f, .color = kText.withAlpha(b), .align = gamelib::draw::Align::Center,
                       .shadow = true});
    if (m_newBestRun) {
        list.text("NEW BEST RUN", cx, 760.0f,
                  {.pixelSize = 5.0f, .color = kHero.withAlpha(b * (0.7f + 0.3f * std::sin(m_endingTime * 6.0f))),
                   .align = gamelib::draw::Align::Center});
    }
}

std::string LeapGame::worldBestLine(const int world) const
{
    const gamelib::core::LevelRecord& record = m_profile.value().game.campaign.record(worldLevel(world));
    if (!record.cleared || !record.bestTicks) {
        return "WORLD " + std::to_string(world + 1) + " NOT CLEARED YET";
    }
    return "BEST " + formatTime(*record.bestTicks) + "   FEWEST DEATHS " + std::to_string(record.fewestDeaths.value_or(0));
}

void LeapGame::drawMenus(gamelib::draw::DrawList& list, const gamelib::core::Size canvas) const
{
    const gamelib::ui::Menu* menu = m_menus.menu();
    if (menu == nullptr) {
        return;
    }
    std::string subtitle;
    std::string footer;
    if (m_menus.is(ArcadeScreen::Title)) {
        subtitle = worldBestLine(m_profile.value().game.world);
        footer = "RUN: ARROWS / A D    JUMP: SPACE / Z / C    PAUSE: ESC    FULLSCREEN: F11";
    } else if (m_menus.is(ArcadeScreen::GameOver) && m_session) {
        subtitle = "TIME " + formatTime(m_session->ticks()) + "   DEATHS " + std::to_string(m_session->deaths());
        const auto& bests = m_profile.value().bests;
        if (bests.get(kRunTime) > 0) {
            footer = "BEST RUN " + formatTime(static_cast<int>(bests.get(kRunTime))) + "   FEWEST DEATHS "
                     + std::to_string(bests.get(kRunDeaths) - 1);
        }
    } else if (m_menus.is(ArcadeScreen::Paused) && m_session) {
        subtitle = std::string{game::worldName(paletteWorld(m_session->room()))} + "  "
                   + std::string{m_session->roomDef().name};
    }
    gamelib::ui::drawMenu(list, *menu, canvas, _menuStyle(), subtitle, footer);
}

void LeapGame::drawDebug(gamelib::draw::DrawList& list, const gamelib::core::Size canvas, const game::Session& session)
{
    if (!m_debug.enabled()) {
        m_debug.draw(list);
        return;
    }
    const gamelib::core::View2D view = roomView(canvas);
    // The tile grid, hazards and the hero's real hitbox (smaller than its drawing).
    for (int x = 0; x <= game::kColumns; ++x) {
        const Vec2 a = view.toCanvas({static_cast<float>(x) * game::kTile, 0.0f});
        m_debug.line(a, {a.x, static_cast<float>(canvas.height)}, Color{1.0f, 1.0f, 1.0f, 0.08f}, 1.0f);
    }
    for (int y = 0; y <= game::kRows; ++y) {
        const Vec2 a = view.toCanvas({0.0f, static_cast<float>(y) * game::kTile});
        m_debug.line(a, {static_cast<float>(canvas.width), a.y}, Color{1.0f, 1.0f, 1.0f, 0.08f}, 1.0f);
    }
    m_debug.box(view.rectToCanvas(session.hero().box), Color{0.3f, 1.0f, 0.4f, 1.0f});
    for (const game::Saw& saw : session.saws()) {
        const float r = saw.radius * game::kSawHitShrink;
        m_debug.box(view.rectToCanvas(gamelib::core::Rect::fromCenter(saw.position, {2.0f * r, 2.0f * r})),
                    Color{1.0f, 0.3f, 0.3f, 1.0f});
    }
    m_debug.stat("LOOP", "TICKS " + std::to_string(m_frameTicks) + (m_focusPaused ? " FOCUS PAUSED" : ""));
    const auto& hero = session.hero();
    m_debug.stat("ROOM", std::to_string(session.room()) + " " + std::string{session.roomDef().name});
    m_debug.stat("POS", std::to_string(static_cast<int>(hero.box.x)) + "," + std::to_string(static_cast<int>(hero.box.y)));
    m_debug.stat("VEL", std::to_string(hero.velocity.x).substr(0, 5) + "," + std::to_string(hero.velocity.y).substr(0, 5));
    m_debug.stat("STATE", std::string{hero.grounded ? "GROUND" : "AIR"} + (hero.wallSliding ? " SLIDE" : "")
                              + " COYOTE " + std::to_string(hero.coyote) + " BUFFER " + std::to_string(hero.jumpBuffer));
    m_debug.draw(list);
}

} // namespace leap::app
