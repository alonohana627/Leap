# GameLib revision

Leap and GameLib are separate repositories, and commits across repositories
are not atomic. This file records which GameLib revision each Leap change was
validated against (build with zero warnings, smoke run with the validation
layer). `CMakeLists.txt` pins GameLib by full commit hash (`GIT_TAG`,
`GIT_SHALLOW FALSE`), so checking out a Leap commit reproduces its
combination by itself; CMake prints the revision it builds against
(`Leap: using GameLib ...`).

| Leap change | GameLib revision | Validated |
|---|---|---|
| Project from the new-game template (`build`) | `dc4d82b` (0.0.6 plus docs) | clean debug build fetching the commit, zero warnings, 1/1 smoke run |
| The rules and the 33 rooms, the playable game with shaders, menus, sound and music, harder worlds 3-4, fixes (`feat(game)`, `feat(app)`, `fix`) | `dc4d82b` | debug and release builds with zero compiler and shader warnings; 1/1 smoke run; a `--demo` self-test (6000 frames with a resize, a minimize and a focus loss) and the whole campaign played from a recording in the debug build, zero validation messages; every room finished by a headless search driver |
