#pragma once

// The attract mode's built-in demo: a recorded run through world 1 (rooms
// 1-1 to 1-8, no deaths), as core::InputRecording text. It was recorded by
// driving this game's own rules (Session) with a search over per-tick inputs
// outside the repository, so it replays exactly while kRulesVersion and the
// world 1 rooms stay as they are; regenerate it when either changes. The
// player's own fastest world 1 run replaces it once there is one
// (app/profile.hpp, leap-demo.txt).

#include <string_view>

namespace leap::game {

[[nodiscard]] std::string_view builtinDemo();

} // namespace leap::game
