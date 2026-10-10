// Leap - a single-screen precision platformer campaign on GameLib.
//
// Command line: --demo (the recorded world 1 run plays full screen, looping)
// plus GameLib's self-test switches (--frames N --hidden --require-validation
// --no-vsync --allow-software --resize-at-frame N --minimize-at-frame N
// --focus-loss-at-frame N).
#include "leap_game.hpp"
#include "profile.hpp"
#include "rules.hpp"

#include <gamelib/app/app.hpp>

#include <memory>
#include <utility>

int main(int argc, char** argv)
{
    // Loaded before the window exists, so a saved fullscreen setting is
    // applied while the window is still hidden. LEAP_DATA_DIR moves the file
    // (the smoke run keeps its own in the build folder).
    leap::app::ProfileFile profile = leap::app::openProfile();
    gamelib::app::AppConfig config{
        .title = "Leap",
        .windowSize = {1280, 720},
        .canvas = {1920, 1080},
        .tick = std::chrono::nanoseconds{1'000'000'000 / leap::game::kTickRate},
        .fullscreen = profile.value().settings.fullscreen,
        .f11TogglesFullscreen = false, // F11 goes through ArcadeMenus, so the setting is saved
        .pauseOnFocusLoss = true,      // a precision game must not run on while the player is away
        .gameFlags = {"--demo"},
    };
    config.renderer.vsync = profile.value().settings.vsync;
    return gamelib::app::runMain(argc, argv, config, [&](gamelib::app::App& app) {
        return std::make_unique<leap::app::LeapGame>(app, std::move(profile), app.hasFlag("--demo"));
    });
}
