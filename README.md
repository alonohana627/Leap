# Leap

A single-screen precision platformer campaign built on
[GameLib](https://github.com/alonohana627/GameLib), in the spirit of
Celeste's rooms and Super Meat Boy: four worlds of eight rooms and a summit.
Run, jump, slide down walls and jump off them, ride springs, cross crumbling
blocks and slip past saws. Dying is instant and so is coming back.

Everything on screen is drawn by shaders (neon tiles, a blob hero, saws,
flags, animated backdrops); every sound and the music are synthesized at
startup. No image or audio files.

## Controls

| Action | Keyboard | Gamepad |
|---|---|---|
| Run | Arrows or A / D | D-pad or left stick |
| Jump (hold to jump higher) | Space, Z, C, K, Up or W | A or B |
| Pause | Escape or P | Start |
| Fullscreen | F11 | |
| Debug overlay (hitboxes, tile grid) | F3 | |

Jumping while touching a wall (from world 2) pushes off it. A jump pressed
just before landing, or just after running off a ledge, still counts.

## Worlds

1. **Foothills**: running and jumping, one idea per room.
2. **Cliffs**: wall slides and wall jumps; chimneys are left over their shorter wall.
3. **Quarry**: crumbling blocks and springs.
4. **Foundry**: saws on fixed tracks, with everything before.
5. **The summit**: the end of the run, with its time and deaths.

The title screen picks any world already opened. `leap.ini` in the user data
folder (`%APPDATA%\Leap` on Windows; `LEAP_DATA_DIR` moves it) keeps the
settings, the open worlds, each world's best time and fewest deaths and the
best full run. Your fastest run through world 1 becomes the demo that plays
behind the title (`leap-demo.txt`).

## Build and run

Requirements are GameLib's (C++26 Clang, CMake 3.28+, Ninja, Vulkan 1.3,
glslc). CMake downloads GameLib at the commit pinned in `CMakeLists.txt`.

```powershell
cmake --preset debug
cmake --build --preset debug
build/debug/bin/leap.exe            # --demo plays the recorded world 1 run full screen
ctest --preset debug                # smoke run with the validation layer
```
