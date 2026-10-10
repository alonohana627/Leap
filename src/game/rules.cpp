#include "rules.hpp"

namespace leap::game {

std::string encodeInput(const Input& input)
{
    std::string text = "...";
    text[0] = input.move < 0 ? 'L' : (input.move > 0 ? 'R' : '.');
    text[1] = input.jumpHeld ? 'h' : '.';
    text[2] = input.jumpPressed ? 'p' : '.';
    return text;
}

std::optional<Input> decodeInput(const std::string_view text)
{
    if (text.size() != 3 || (text[1] != 'h' && text[1] != '.') || (text[2] != 'p' && text[2] != '.')) {
        return std::nullopt;
    }
    Input input;
    switch (text[0]) {
    case 'L': input.move = -1; break;
    case 'R': input.move = 1; break;
    case '.': break;
    default: return std::nullopt;
    }
    input.jumpHeld = text[1] == 'h';
    input.jumpPressed = text[2] == 'p';
    return input;
}

gamelib::core::PlatformerTuning tuningFor(const int world)
{
    gamelib::core::PlatformerTuning t;
    // Running: about 11 tiles a second, full speed in 6 ticks, stops in 4.
    t.maxSpeed = 3.0f;
    t.groundAccel = 0.5f;
    t.groundDecel = 0.75f;
    t.airAccel = 0.4f;
    t.airDecel = 0.25f;
    // A held jump rises 3.25 tiles and peaks after 19 ticks (0.32 s); falls
    // are heavier than rises, with a floatier apex while jump is held.
    t.setJumpArc(3.25f * kTile, 19);
    t.fallGravity = 1.5f;
    t.maxFallSpeed = 6.5f;
    t.jumpCut = 0.45f;
    t.apexThreshold = 1.0f;
    t.apexGravity = 0.5f;
    t.coyoteTicks = 6;
    t.jumpBufferTicks = 6;
    t.cornerCorrection = 4.0f;
    t.hazardInset = 3.0f;
    if (world >= 1) {
        // Wall slide and wall jump from world 2: a push off the wall that
        // the input cannot cancel for 9 ticks.
        t.wallSlideSpeed = 1.6f;
        t.wallJumpSpeed = {3.4f, 5.4f};
        t.wallJumpLockTicks = 9;
        t.wallJumpDistance = 3.0f;
    }
    return t;
}

} // namespace leap::game
