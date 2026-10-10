# leap_app: everything around the rules - the GameLib main-loop glue, the
# profile file and demo recording, the view and its shaders, sounds, music
# and effects. Menus, settings, sound banks, transitions, effects and the
# attract mode come from GameLib (app, ui, audio, fx, ai).
add_library(leap_app STATIC
    effects.cpp
    leap_game.cpp
    profile.cpp
    sounds.cpp
    view.cpp)
target_include_directories(leap_app PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(leap_app PUBLIC leap_game GameLib::app GameLib::ui GameLib::fx GameLib::ai)
gamelib_target_warnings(leap_app)
# Leap's own procedural shapes, embedded as SPIR-V (see view.hpp, Shapes).
gamelib_add_shaders(leap_app SHADERS
    ${PROJECT_SOURCE_DIR}/shaders/backdrop.frag
    ${PROJECT_SOURCE_DIR}/shaders/flag.frag
    ${PROJECT_SOURCE_DIR}/shaders/hero.frag
    ${PROJECT_SOURCE_DIR}/shaders/saw.frag
    ${PROJECT_SOURCE_DIR}/shaders/tile.frag)
