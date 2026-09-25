cmake_minimum_required(VERSION 3.16)
project(free_world_appearance_resolve_v0_7 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

get_filename_component(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
set(APP_CPP "${REPO_ROOT}/suite_android/app/src/main/cpp")
set(FREE_WORLD_V02 "${REPO_ROOT}/docs/research/free-world-pixel-resolve-2d-v0.2/native")
set(DEEP_V04 "${REPO_ROOT}/docs/research/free-world-deep-scene-contribution-v0.4/native")

add_executable(test_free_world_appearance_resolve_v0_7
    tests/test_free_world_appearance_resolve_v0_7.cpp
    native/free_world_appearance_resolve_v0_7.cpp
    "${DEEP_V04}/free_world_deep_scene_contribution_v0_4.cpp"
    "${FREE_WORLD_V02}/free_world_pixel_resolve_2d_v0_2.cpp"
    "${APP_CPP}/truthraw_sha256_v0_69.cpp"
)

target_include_directories(test_free_world_appearance_resolve_v0_7 PRIVATE
    native
    "${DEEP_V04}"
    "${FREE_WORLD_V02}"
    "${APP_CPP}"
)

if(MSVC)
  target_compile_options(test_free_world_appearance_resolve_v0_7 PRIVATE /W4 /WX)
else()
  target_compile_options(test_free_world_appearance_resolve_v0_7 PRIVATE
      -Wall -Wextra -Wpedantic -Werror -ffp-contract=off -fno-fast-math)
endif()

enable_testing()
add_test(NAME free_world_appearance_resolve_v0_7
         COMMAND test_free_world_appearance_resolve_v0_7)
