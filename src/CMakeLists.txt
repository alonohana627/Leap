add_subdirectory(game)
add_subdirectory(app)

# The executable is only main.cpp; everything else lives in the libraries.
add_executable(leap main.cpp)
target_link_libraries(leap PRIVATE leap_app)
gamelib_target_warnings(leap)
gamelib_copy_runtime(leap)
