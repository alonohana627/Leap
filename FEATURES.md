# Features

What Leap does today, verified by running it. Updated with each feature.

## The campaign

- Four worlds of eight single-screen rooms and a summit room (33 rooms),
  written as 32 x 18 text grids in `src/game/rooms.cpp` and read with
  GameLib's `core::TileMap::fromText`: FOOTHILLS (running and jumping, one
  idea per room with a teaching line), CLIFFS (wall slides and wall jumps),
  QUARRY (crumbling blocks and springs), FOUNDRY (saws on fixed tracks with
  everything before) and the SUMMIT.
- Leaving a room through its exit gap wipes to the next room in the exit's
  direction; leaving world 4 leads to the summit, whose beacon ends the run.
- Every room is proven finishable with the real rules (a headless search
  driver outside the repository; the full campaign also played through the
  game itself from a recording).

## Movement (GameLib's platformer controller, Leap's tuning, 60 ticks a second)

- Acceleration and braking on the ground and in the air, a capped fall
  speed, falls heavier than rises and a gentler apex while jump is held.
- Variable jump height (releasing jump cuts the rise), coyote time (6 ticks)
  and a jump buffer (6 ticks), corner correction (4 units) when the head
  clips a ledge.
- Wall slide and wall jump from world 2, with a short input lock so the push
  off the wall carries.
- Keyboard (arrows or A/D, jump on Space, Z, C, K, Up or W) and gamepad
  (d-pad or left stick, A or B); a jump press counts once even when a frame
  runs several ticks.

## Hazards and objects

- Spikes in four directions with hitboxes smaller than their drawing, pits,
  crumbling blocks that shake, fall a moment after being stood or slid on
  and come back (never on top of the hero), springs that launch about eight
  tiles high, saws ping-ponging or looping on dotted tracks.
- A death bursts the hero, flashes and shakes the screen and respawns it in
  0.4 s at the room's entry or at the last checkpoint flag reached in the
  room; the room's objects reset with it.

## Progress and screens

- Title screen with PLAY, a WORLD select (open worlds only), sound, music,
  screen shake, fullscreen and quit (GameLib's `ArcadeMenus`); the selected
  world's best time and fewest deaths under the title.
- The attract demo behind the title replays a recorded run through world 1
  with the real rules; the player's fastest run through world 1 (recorded
  tick by tick) replaces the built-in recording. `--demo` shows it full
  screen.
- HUD with the room, the run's time and deaths; banners for each world's
  start and clear (time, deaths, new best time or fewest deaths).
- Pause menu (Escape, P or the gamepad's Start; also when the window loses
  focus) with resume, settings, restart, main menu and quit.
- The summit's ending: fireworks, the run's time and deaths, "new best run",
  then PLAY AGAIN / MAIN MENU under "THE SUMMIT" with the best run.
- `leap.ini` in the user data folder (`LEAP_DATA_DIR` moves it): standard
  settings, worlds unlocked, per world the best time and fewest deaths, the
  best full run; `leap-demo.txt` next to it holds the demo recording.

## Look and sound

- Neon on dark, all drawn by shaders: one tile shader for rock (rounded and
  outlined only where a neighbour is empty, a soft inner glow), spikes,
  crumbling blocks (cracks that widen) and springs; a blob hero with eyes
  that blinks, looks where it goes and squashes and stretches; spinning saw
  blades; waving checkpoint flags and the summit beacon; an animated
  backdrop per world (hills and aurora, cliffs and rain, strata and dust,
  girders and sparks, stars and aurora); bloom on the edges.
- Dust puffs on jumps, landings and wall slides, rings on springs and
  respawns, debris from crumbling blocks, fireworks at the summit.
- Synthesized jump, wall jump, land, wall-slide hiss (looped while sliding),
  spring, crumble, death, respawn, checkpoint, room wipe, world clear,
  summit and menu sounds, and a looping chiptune per world plus the
  summit's (also the title's).
- F3 toggles GameLib's debug overlay: frame times, the tile grid, the hero's
  hitbox and the saws' hit circles, position, velocity and controller state.
