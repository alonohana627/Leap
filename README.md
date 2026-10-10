# Leap

A single-screen precision platformer campaign built on
[GameLib](https://github.com/alonohana627/GameLib): four worlds of eight rooms
and a summit, in the spirit of Celeste's rooms and Super Meat Boy. Work in
progress.

## Build and run

Requirements are GameLib's (C++26 Clang, CMake 3.28+, Ninja, Vulkan 1.3,
glslc). CMake downloads GameLib at the commit pinned in `CMakeLists.txt`.

```powershell
cmake --preset debug
cmake --build --preset debug
build/debug/bin/leap.exe
ctest --preset debug    # smoke run with the validation layer
```
