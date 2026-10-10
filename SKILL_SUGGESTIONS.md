# Skill suggestions

Skills that would have made work on this project go better. See `AGENTS.md` for
how this file is maintained.

## Already installed

In `~/.claude/skills/` (checked 2026-10-10):

- action-rpg-progression-design
- arcade-action-physics
- chiptune-sfx-synthesis
- ci-cross-platform
- cmake-build-developer
- cmake-library-consumption
- computer-graphics-developer
- cozy-sim-design
- cpp-debugging-profiling
- cpp-performance-developer
- cpp-testing
- cpp26-developer
- deterministic-replay-testing
- game-audio-developer
- game-engine-developer
- game-feel-designer
- game-library-extraction
- game-persistence
- game-ui-text-and-dialogue
- game-visual-design
- glfw-platform-developer
- glsl-shader-build-embedding
- grid-algorithms-for-games
- headless-balance-sweeps
- learning-mentor
- logic-puzzle-demo-players
- metroidvania-world-design
- platformer-controller-feel
- pong-arcade-rules
- procedural-dungeon-generation
- puzzle-game-design-undo
- retro-crt-screen-shader
- sdf-2d-shape-shaders
- shader-developer
- software-architecture-developer
- technical-docs-writer
- tilemap-and-camera-2d
- vector-arcade-neon-look
- vulkan-app-smoke-testing
- vulkan-developer
- vulkan-offscreen-postprocess
- vulkan-text-and-ui
- win32-game-window-automation
- window-screenshot-bursts
- windows-release-packaging

## Suggested


### platformer-route-search

How to prove that hand-made platformer levels can be finished, and to record
demo runs, by searching over per-tick inputs of the game's own deterministic
simulation: a beam search whose actions are short macros (run left, right or
not, jump released, tapped, held for N ticks), states deduplicated by
quantized position, velocity, grounded and timer state (plus the time bucket
when hazards move), scored by a flood-fill distance to the exit through open
cells, optional waypoints for routes that first lead away from the exit, a
smoothness penalty so recorded demos do not twitch; how to read the result as
a difficulty measure (the coarsest decision interval that still finishes a
room), chain room solutions into one recording and verify it by replaying the
whole campaign with a final state hash; and the pitfalls (greedy scores that
prune the run-up a long jump needs, the beam width that hides them).

*Why:* Leap's 33 rooms were verified, tuned for difficulty and given their
attract demo by such a driver, written from scratch outside the repository;
every later platformer of the run needs the same proof that its levels are
fair.

### precision-platformer-room-design

Designing single-screen rooms for a precision platformer from the
controller's numbers: jump height and running jump distance turned into the
largest gap and step a room may ask for, chimneys for wall jumps (a two-wall
shaft is left over its shorter wall; without a wall grab a single wall cannot
be climbed), spikes with hitboxes smaller than the drawing, crumbling blocks
and springs as timing, saws on tracks as windows, one new idea per early room,
checkpoint placement in long rooms, a difficulty curve over a world, hint text
that never sits under tiles, and checking every room in a screenshot and with
a solver.

*Why:* Leap's world 2 rooms first assumed single-wall climbing, which the
controller cannot do, and three teaching lines were placed inside rock;
both had to be found by running the game. Celeste-style room craft is not
covered by `metroidvania-world-design` (connected maps) or
`platformer-controller-feel` (the controller itself).
