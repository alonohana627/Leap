#pragma once

// Leap on GameLib's main loop: the title (world select, the attract demo
// replaying a recorded run through world 1), the run itself with a wipe
// between rooms, pause, the world-clear banners and the summit's ending.
// The rules live in src/game; this is input, presentation, sound and saves.

#include "effects.hpp"
#include "profile.hpp"
#include "session.hpp"
#include "sounds.hpp"
#include "view.hpp"

#include <gamelib/ai/attract.hpp>
#include <gamelib/app/app.hpp>
#include <gamelib/app/arcade_menus.hpp>
#include <gamelib/audio/sound_bank.hpp>
#include <gamelib/fx/debug_overlay.hpp>
#include <gamelib/fx/transients.hpp>
#include <gamelib/ui/transition.hpp>

#include <memory>
#include <optional>
#include <string>

namespace leap::app {

class LeapGame final : public gamelib::app::Game {
public:
    // demo: the attract demo plays full screen without the menu (--demo).
    LeapGame(gamelib::app::App& app, ProfileFile profile, bool demo);

    void onFocus(gamelib::app::App& app, bool focused) override;
    void onKey(gamelib::app::App& app, const gamelib::platform::KeyEvent& event) override;
    void onGamepad(gamelib::app::App& app, const gamelib::platform::GamepadEvent& event) override;
    void tick(gamelib::app::App& app) override;
    void frame(gamelib::app::App& app, const gamelib::app::FrameInfo& info) override;
    void draw(gamelib::app::App& app, gamelib::draw::DrawList& list, const gamelib::app::FrameInfo& info) override;

private:
    // The attract demo: a session fed from a recording.
    struct DemoRun {
        game::Session session;
        std::shared_ptr<const Recording> recording;
        std::size_t next = 0;
    };
    struct Banner {
        std::string title;
        std::string line;
        gamelib::core::Color color;
    };

    void handleResult(gamelib::app::App& app, const gamelib::app::ArcadeResult& result);
    void startRun(gamelib::app::App& app);
    void leaveRun(gamelib::app::App& app);
    [[nodiscard]] game::Input readInput(const gamelib::platform::InputState& input);
    void playTick(gamelib::app::App& app);
    void demoTick();
    void react(gamelib::app::App& app, const game::Event& event);
    void startWipe(gamelib::ui::Transition& transition, game::Side side, std::function<void()> atSwitch);
    void announceWorld(int room);
    void updateAudio(gamelib::app::App& app);
    [[nodiscard]] bool showsRun() const;
    void drawHud(gamelib::draw::DrawList& list, gamelib::core::Size canvas, const game::Session& session) const;
    void drawBanners(gamelib::draw::DrawList& list, gamelib::core::Size canvas) const;
    void drawEnding(gamelib::draw::DrawList& list, gamelib::core::Size canvas) const;
    void drawMenus(gamelib::draw::DrawList& list, gamelib::core::Size canvas) const;
    void drawDebug(gamelib::draw::DrawList& list, gamelib::core::Size canvas, const game::Session& session);
    [[nodiscard]] std::string worldBestLine(int world) const;

    ProfileFile m_profile; // the menus edit its settings and world in place: declared first
    gamelib::app::ArcadeMenus m_menus;
    gamelib::audio::SoundBank<Sfx, kSfxCount> m_sfx;
    Music m_music;
    Shapes m_shapes;

    std::optional<game::Session> m_session; // the player's run
    Effects m_effects;
    gamelib::ui::Transition m_wipe;
    Recording m_recording;          // the current run's input, while it can become the world 1 demo
    bool m_recordingWorldOne = false;
    bool m_pressTaken = false;      // the frame's jump press went to its first tick
    bool m_held = false;            // no tick ran this frame (draw without interpolation)
    bool m_ending = false;
    float m_endingTime = 0.0f;
    float m_nextFirework = 0.0f;
    bool m_newBestRun = false;
    int m_startRoom = -1;           // LEAP_START_ROOM: a developer's start in any room
    std::optional<Recording> m_replay; // LEAP_REPLAY: a recorded input file plays instead of the keyboard
    std::size_t m_replayNext = 0;

    std::shared_ptr<const Recording> m_demoRecording;
    gamelib::ai::AttractMode<DemoRun> m_attract;
    Effects m_demoEffects;
    gamelib::ui::Transition m_demoWipe;
    bool m_demoMode = false;

    gamelib::fx::Transients<Banner> m_banners;
    gamelib::fx::DebugOverlay m_debug;
    float m_time = 0.0f;
    float m_hintFade = 0.0f;
};

} // namespace leap::app
