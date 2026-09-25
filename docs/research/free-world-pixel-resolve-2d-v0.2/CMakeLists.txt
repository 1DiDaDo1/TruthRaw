cmake_minimum_required(VERSION 3.16)
project(free_world_pixel_resolve_2d_v0_2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(test_free_world_pixel_resolve_2d_v0_2
    tests/test_free_world_pixel_resolve_2d_v0_2.cpp
    native/free_world_pixel_resolve_2d_v0_2.cpp
)

target_include_directories(test_free_world_pixel_resolve_2d_v0_2 PRIVATE
    native
)

if(MSVC)
  target_compile_options(test_free_world_pixel_resolve_2d_v0_2 PRIVATE /W4 /WX)
else()
  target_compile_options(test_free_world_pixel_resolve_2d_v0_2 PRIVATE
      -Wall -Wextra -Wpedantic -Werror -ffp-contract=off -fno-fast-math)
endif()

enable_testing()
add_test(NAME free_world_pixel_resolve_2d_v0_2
         COMMAND test_free_world_pixel_resolve_2d_v0_2)
