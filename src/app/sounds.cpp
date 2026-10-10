#include "sounds.hpp"

#include <gamelib/audio/synth.hpp>

namespace leap::app {

namespace {

// helpers

using gamelib::audio::Note;
using gamelib::audio::Song;
using gamelib::audio::Wave;
using gamelib::audio::noise;
using gamelib::audio::tone;

// MIDI note numbers; the effects sit around C major so they agree with the
// first world's song.
constexpr float kC3 = 48, kG3 = 55, kC4 = 60, kE4 = 64, kG4 = 67, kA4 = 69, kC5 = 72, kD5 = 74, kE5 = 76;
constexpr float kG5 = 79, kA5 = 81, kC6 = 84, kE6 = 88, kG6 = 91, kC7 = 96;

struct Recipe {
    std::vector<std::vector<Note>> tracks;
    float peakDb = -14.0f; // loudest sample, dBFS
};

Recipe _recipe(const Sfx sfx)
{
    switch (sfx) {
    // A quick rising blip: the classic jump.
    case Sfx::Jump: return {{{tone(Wave::Pulse25, kC5, 0.09f, kG5)}}, -19.0f};
    // A wall jump: the blip with a scrape of noise under it.
    case Sfx::WallJump:
        return {{{tone(Wave::Pulse25, kD5, 0.09f, kA5)}, {noise(6000.0f, 0.05f, 2500.0f, 0.5f)}}, -18.0f};
    // A soft thud: low triangle drop and a puff of noise.
    case Sfx::Land: return {{{tone(Wave::Triangle, kG3, 0.06f, kC3)}, {noise(1800.0f, 0.04f, 600.0f, 0.6f)}}, -20.0f};
    // A long even hiss, looped while sliding: no decay towards its end.
    case Sfx::SlideHiss:
        return {{{noise(9000.0f, 0.25f, 9000.0f, 1.0f), noise(8000.0f, 0.25f, 9000.0f, 1.0f)}}, -27.0f};
    // A boing: a fast pitch sweep up with a wobble back.
    case Sfx::Spring:
        return {{{tone(Wave::Pulse50, kC4, 0.07f, kC6), tone(Wave::Pulse50, kG5, 0.12f, kE6, 0.7f)},
                 {tone(Wave::Triangle, kC3, 0.15f, kG3)}},
                -15.0f};
    // A dry crackle: the block starts to give.
    case Sfx::CrumbleShake:
        return {{{noise(2200.0f, 0.05f, 1500.0f, 0.8f), noise(2600.0f, 0.05f, 1200.0f, 0.6f),
                  noise(2000.0f, 0.06f, 900.0f, 0.5f)}},
                -23.0f};
    // Rubble falling away.
    case Sfx::CrumbleBreak:
        return {{{noise(1400.0f, 0.22f, 300.0f)}, {tone(Wave::Triangle, kG3, 0.12f, kC3 - 12.0f, 0.6f)}}, -17.0f};
    // A pop and a short falling sting: quick, so dying a lot never grates.
    case Sfx::Death:
        return {{{noise(8000.0f, 0.18f, 500.0f)},
                 {tone(Wave::Pulse25, kE5, 0.06f, kC5), tone(Wave::Pulse25, kG4, 0.12f, kC4 - 5.0f)}},
                -13.0f};
    // Back: a soft upward shimmer.
    case Sfx::Respawn:
        return {{{tone(Wave::Pulse12, kC5, 0.04f), tone(Wave::Pulse12, kE5, 0.04f), tone(Wave::Pulse12, kG5, 0.07f)}},
                -22.0f};
    // A bright two-note chime.
    case Sfx::Checkpoint:
        return {{{tone(Wave::Pulse25, kG5, 0.08f), tone(Wave::Pulse25, kC6, 0.22f)},
                 {tone(Wave::Triangle, kC4, 0.3f)}},
                -14.0f};
    // A whoosh for the wipe between rooms.
    case Sfx::RoomExit: return {{{noise(3000.0f, 0.28f, 9000.0f, 0.7f)}}, -24.0f};
    // World clear: a rising arpeggio and a held chord.
    case Sfx::WorldClear:
        return {{{tone(Wave::Pulse25, kC5, 0.08f), tone(Wave::Pulse25, kE5, 0.08f), tone(Wave::Pulse25, kG5, 0.08f),
                  tone(Wave::Pulse25, kC6, 0.08f), tone(Wave::Pulse25, kE6, 0.08f), tone(Wave::Pulse25, kG6, 0.4f)},
                 {tone(Wave::Triangle, kC3, 0.32f), tone(Wave::Triangle, kG3, 0.48f)}},
                -12.0f};
    // The summit: a longer fanfare.
    case Sfx::Summit:
        return {{{tone(Wave::Pulse25, kG4, 0.12f), tone(Wave::Pulse25, kC5, 0.12f), tone(Wave::Pulse25, kE5, 0.12f),
                  tone(Wave::Pulse25, kG5, 0.24f), tone(Wave::Pulse25, kE5, 0.12f), tone(Wave::Pulse25, kG5, 0.12f),
                  tone(Wave::Pulse25, kC6, 0.7f)},
                 {tone(Wave::Pulse12, kE4, 0.36f), tone(Wave::Pulse12, kG4, 0.36f), tone(Wave::Pulse12, kA4, 0.24f),
                  tone(Wave::Pulse12, kG4, 0.7f)},
                 {tone(Wave::Triangle, kC3, 0.72f), tone(Wave::Triangle, kG3, 0.24f), tone(Wave::Triangle, kC3, 0.7f)}},
                -11.0f};
    case Sfx::MenuMove: return {{{tone(Wave::Pulse12, kC7, 0.03f)}}, -20.0f};
    case Sfx::MenuSelect: return {{{tone(Wave::Pulse25, kC5, 0.04f), tone(Wave::Pulse25, kG5, 0.07f)}}, -16.0f};
    case Sfx::MenuBack: return {{{tone(Wave::Pulse25, kG5, 0.04f), tone(Wave::Pulse25, kC5, 0.07f)}}, -16.0f};
    case Sfx::Count: break;
    }
    return {};
}

} // namespace

std::vector<float> renderSfx(const Sfx sfx, const std::uint32_t sampleRate)
{
    const Recipe recipe = _recipe(sfx);
    return gamelib::audio::renderMix(recipe.tracks, recipe.peakDb, sampleRate);
}

Song song(const int index)
{
    switch (index) {
    case 0: // FOOTHILLS: bright C major, walking pace.
        return {.bpm = 140.0f,
                .tracks = {{.wave = Wave::Pulse25, .volume = 0.8f,
                            .loop = "C5 - E5 - G5 - E5 - D5 - - - C5 - D5 - | E5 - - - G5 - A5 - G5 - - - E5 - - - | "
                                    "F5 - A5 - C6 - A5 - G5 - E5 - D5 - C5 - | D5 - - - G4 - B4 - C5 - - - . . . ."},
                           {.wave = Wave::Triangle,
                            .loop = "C3 - . C3 C3 - G2 - C3 - . C3 C3 - G2 - | A2 - . A2 A2 - E2 - A2 - . A2 A2 - E2 - | "
                                    "F2 - . F2 F2 - C3 - F2 - . F2 F2 - C3 - | G2 - . G2 G2 - D3 - G2 - . G2 B2 - D3 -"},
                           {.wave = Wave::Noise, .volume = 0.35f,
                            .loop = "C4 . C8 . C6 . C8 . C4 . C8 C4 C6 . C8 ."}},
                .peakDb = -17.0f};
    case 1: // CLIFFS: A minor, brisk.
        return {.bpm = 150.0f,
                .tracks = {{.wave = Wave::Pulse25, .volume = 0.8f,
                            .loop = "A4 - C5 - E5 - A5 - G5 - E5 - D5 - E5 - | C5 - - - B4 - - - A4 - - - E4 - - - | "
                                    "F4 - A4 - C5 - F5 - E5 - C5 - B4 - G4 - | A4 - - - - - - - E5 - D5 - C5 - B4 -"},
                           {.wave = Wave::Triangle,
                            .loop = "A2 - A2 . E3 - A2 . A2 - A2 . G2 - E2 - | A2 - A2 . E3 - A2 . A2 - A2 . E2 - G2 - | "
                                    "F2 - F2 . C3 - F2 . F2 - F2 . G2 - G2 - | A2 - A2 . E3 - A2 . E2 - E2 . E2 - G#2 -"},
                           {.wave = Wave::Noise, .volume = 0.35f,
                            .loop = "C3 . C8 C8 C6 . C8 . C3 C3 C8 . C6 . C8 C8"}},
                .peakDb = -17.0f};
    case 2: // QUARRY: D dorian, a slower groove.
        return {.bpm = 128.0f,
                .tracks = {{.wave = Wave::Pulse25, .volume = 0.8f,
                            .loop = "D5 - . D5 F5 - G5 - A5 - . G5 F5 - D5 - | C5 - . C5 D5 - F5 - G5 - - - . . . . | "
                                    "D5 - . D5 F5 - G5 - A5 - C6 - B5 - G5 - | A5 - - - G5 - F5 - E5 - - - D5 - - -"},
                           {.wave = Wave::Triangle,
                            .loop = "D2 - D3 - D2 - D3 - C2 - C3 - C2 - C3 - | D2 - D3 - D2 - D3 - G2 - G3 - A2 - C3 -"},
                           {.wave = Wave::Noise, .volume = 0.35f,
                            .loop = "C2 . C8 . C5 . C8 C8 C2 C2 C8 . C5 . C8 ."}},
                .peakDb = -17.0f};
    case 3: // FOUNDRY: E minor, driving sixteenths.
        return {.bpm = 165.0f,
                .tracks = {{.wave = Wave::Pulse25, .volume = 0.75f,
                            .loop = "E5 - E5 - G5 - E5 - B5 - A5 - G5 - F#5 - | E5 - - - B4 - - - D5 - E5 - - - . . | "
                                    "C5 - C5 - E5 - C5 - G5 - F#5 - E5 - D5 - | B4 - - - D#5 - - - F#5 - - - B5 - - -"},
                           {.wave = Wave::Triangle,
                            .loop = "E2 E2 E3 E2 E2 E2 E3 E2 E2 E2 E3 E2 D2 D2 D3 D2 | "
                                    "C2 C2 C3 C2 C2 C2 C3 C2 B1 B1 B2 B1 B1 B1 B2 D#2"},
                           {.wave = Wave::Noise, .volume = 0.35f,
                            .loop = "C2 C8 C6 C8 C2 C8 C6 C8 C2 C8 C6 C8 C2 C2 C6 C6"}},
                .peakDb = -17.0f};
    default: // SUMMIT (and the title): airy F major, slow.
        return {.bpm = 100.0f,
                .tracks = {{.wave = Wave::Pulse25, .volume = 0.7f,
                            .loop = "F5 - - - A5 - - - C6 - - - B5 - - - | A5 - - - G5 - - - F5 - - - - - - - | "
                                    "D5 - - - F5 - - - A5 - - - G5 - E5 - | F5 - - - - - - - . . . . . . . ."},
                           {.wave = Wave::Pulse12, .volume = 0.3f,
                            .loop = "F4 A4 C5 A4 F4 A4 C5 A4 G4 B4 D5 B4 G4 B4 D5 B4"},
                           {.wave = Wave::Triangle,
                            .loop = "F2 - - - - - - - G2 - - - - - - - | D2 - - - - - - - C2 - - - - - - -"},
                           {.wave = Wave::Noise, .volume = 0.25f, .loop = "C7 . . . C7 . . . C7 . . . C7 . C7 ."}},
                .peakDb = -17.0f};
    }
}

Music::Music(gamelib::audio::Player& player)
{
    for (int i = 0; i < kSongCount; ++i) {
        const gamelib::audio::RenderedSong rendered = gamelib::audio::renderSong(song(i), player.sampleRate());
        m_tracks[static_cast<std::size_t>(i)] = player.addMusic(rendered.samples, rendered.loopStart);
    }
}

void Music::play(gamelib::audio::Player& player, const int index) const
{
    const gamelib::audio::SoundId track = m_tracks[static_cast<std::size_t>(index < 0 || index >= kSongCount ? 0 : index)];
    if (player.music() != track) {
        player.playMusic(track, 0.8f);
    }
}

} // namespace leap::app
