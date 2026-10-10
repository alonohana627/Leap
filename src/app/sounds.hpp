#pragma once

// Leap's sound effects and music, all synthesized with GameLib's 8-bit synth
// at startup: no audio files.

#include <gamelib/audio/music.hpp>
#include <gamelib/audio/player.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace leap::app {

enum class Sfx : std::uint8_t {
    Jump,
    WallJump,
    Land,
    SlideHiss, // looped while the hero slides down a wall
    Spring,
    CrumbleShake,
    CrumbleBreak,
    Death,
    Respawn,
    Checkpoint,
    RoomExit,
    WorldClear,
    Summit,
    MenuMove,
    MenuSelect,
    MenuBack,
    Count
};
inline constexpr std::size_t kSfxCount = static_cast<std::size_t>(Sfx::Count);

[[nodiscard]] std::vector<float> renderSfx(Sfx sfx, std::uint32_t sampleRate);

// One looping chiptune per world (0-3) and the summit's (4, also the title's).
inline constexpr int kSongCount = 5;
[[nodiscard]] gamelib::audio::Song song(int index);

// The rendered songs, registered as music tracks.
class Music {
public:
    explicit Music(gamelib::audio::Player& player);
    // Crossfades to a world's song (4: the summit and title song); nothing if it already plays.
    void play(gamelib::audio::Player& player, int index) const;

private:
    std::array<gamelib::audio::SoundId, kSongCount> m_tracks{};
};

} // namespace leap::app
