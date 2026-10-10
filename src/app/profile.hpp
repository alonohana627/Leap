#pragma once

// What Leap keeps between sessions, in leap.ini in the user data folder
// (LEAP_DATA_DIR moves it): GameLib's standard settings, the campaign's
// progress (which worlds are open; per world the best time and the fewest
// deaths, as core::Campaign records), the world picked on the title screen,
// and the best full run (time and deaths) as named bests. Next to it,
// leap-demo.txt holds the player's fastest run through world 1, which the
// attract mode replays.

#include "rules.hpp"

#include <gamelib/app/arcade_profile.hpp>
#include <gamelib/core/campaign.hpp>
#include <gamelib/core/replay.hpp>
#include <gamelib/storage/ini.hpp>

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace leap::app {

// The level ids of the four worlds in the campaign ("world1" ...).
[[nodiscard]] std::string worldLevel(int world);
[[nodiscard]] gamelib::core::Campaign makeCampaign();

struct Progress {
    gamelib::core::Campaign campaign = makeCampaign();
    int world = 0; // picked on the title screen, 0-based

    void read(const gamelib::storage::IniFile& ini);
    void write(gamelib::storage::IniFile& ini) const;
    friend bool operator==(const Progress&, const Progress&) = default;
};

using ProfileFile = gamelib::app::ArcadeProfileFileOf<Progress>;

inline constexpr std::string_view kRunTime = "run_ticks";   // best full run, lower is better
inline constexpr std::string_view kRunDeaths = "run_deaths"; // fewest deaths in a full run

[[nodiscard]] ProfileFile openProfile();
// Worlds that can be picked: world 1 and every world whose previous one is cleared.
[[nodiscard]] int unlockedWorlds(const gamelib::core::Campaign& campaign);

using Recording = gamelib::core::InputRecording<game::Input>;
[[nodiscard]] std::filesystem::path demoPath();
[[nodiscard]] std::optional<Recording> loadDemo();
void saveDemo(const Recording& recording);

// "1:02.35" from ticks.
[[nodiscard]] std::string formatTime(int ticks);

} // namespace leap::app
