#include "profile.hpp"

#include <gamelib/app/campaign_file.hpp>
#include <gamelib/storage/files.hpp>

#include <algorithm>
#include <cstdio>
#include <vector>

namespace leap::app {

namespace {

// helpers

constexpr std::string_view kAppName = "Leap";
constexpr std::string_view kDataDirVariable = "LEAP_DATA_DIR";

} // namespace

std::string worldLevel(const int world)
{
    return "world" + std::to_string(world + 1);
}

gamelib::core::Campaign makeCampaign()
{
    std::vector<std::string> levels;
    for (int w = 0; w < game::kWorlds; ++w) {
        levels.push_back(worldLevel(w));
    }
    gamelib::core::Campaign campaign{std::move(levels)};
    campaign.requireInOrder();
    return campaign;
}

void Progress::read(const gamelib::storage::IniFile& ini)
{
    gamelib::app::readCampaign(ini, campaign);
    if (const auto text = ini.get("progress", "world")) {
        if (const auto value = gamelib::storage::parseInt(*text)) {
            world = static_cast<int>(std::clamp<long long>(*value - 1, 0, game::kWorlds - 1));
        }
    }
    world = std::min(world, unlockedWorlds(campaign) - 1);
}

void Progress::write(gamelib::storage::IniFile& ini) const
{
    ini.set("progress", "world", std::to_string(world + 1));
    gamelib::app::writeCampaign(campaign, ini);
}

ProfileFile openProfile()
{
    return gamelib::app::openArcadeProfile<Progress>(kAppName, kDataDirVariable);
}

int unlockedWorlds(const gamelib::core::Campaign& campaign)
{
    int open = 0;
    for (int w = 0; w < game::kWorlds; ++w) {
        if (campaign.isUnlocked(worldLevel(w))) {
            open = w + 1;
        }
    }
    return std::max(open, 1);
}

std::filesystem::path demoPath()
{
    return gamelib::storage::userDataDir(kAppName, kDataDirVariable) / "leap-demo.txt";
}

std::optional<Recording> loadDemo()
{
    const std::optional<std::string> text = gamelib::storage::readFile(demoPath());
    if (!text) {
        return std::nullopt;
    }
    std::optional<Recording> recording = Recording::parse(*text, game::decodeInput);
    if (!recording || recording->version() != game::kRulesVersion || recording->empty()) {
        return std::nullopt; // broken, or recorded under other rules: it would not replay
    }
    return recording;
}

void saveDemo(const Recording& recording)
{
    if (const std::error_code error =
            gamelib::storage::writeFileAtomic(demoPath(), recording.serialize(game::encodeInput))) {
        std::fprintf(stderr, "warning: could not save the demo: %s\n", error.message().c_str());
    }
}

std::string formatTime(const int ticks)
{
    const int hundredths = ticks * 100 / game::kTickRate;
    const int minutes = hundredths / 6000;
    const int seconds = (hundredths / 100) % 60;
    char text[32];
    std::snprintf(text, sizeof text, "%d:%02d.%02d", minutes, seconds, hundredths % 100);
    return text;
}

} // namespace leap::app
