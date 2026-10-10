# leap_game: the rules - rooms, the hero's movement (GameLib's platformer
# controller with Leap's tuning), hazards, springs, crumbling blocks, saws,
# checkpoints and the run's time and deaths. Depends only on GameLib::core:
# no window, GPU, audio or clock, deterministic for the per-tick input.
add_library(leap_game STATIC
    demo_run.cpp
    rooms.cpp
    rules.cpp
    session.cpp)
target_include_directories(leap_game PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(leap_game PUBLIC GameLib::core)
gamelib_target_warnings(leap_game)
