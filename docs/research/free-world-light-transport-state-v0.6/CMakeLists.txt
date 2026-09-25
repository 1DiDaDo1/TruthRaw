cmake_minimum_required(VERSION 3.16)
project(free_world_light_transport_state_v0_6 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

get_filename_component(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
set(APP_CPP "${REPO_ROOT}/suite_android/app/src/main/cpp")
set(FREE_WORLD_V02 "${REPO_ROOT}/docs/research/free-world-pixel-resolve-2d-v0.2/native")
set(DEEP_V04 "${REPO_ROOT}/docs/research/free-world-deep-scene-contribution-v0.4/native")
set(DEEP_V05 "${REPO_ROOT}/docs/research/free-world-deep-scene-binding-v0.5/native")

add_executable(test_free_world_light_transport_state_v0_6
    tests/test_free_world_light_transport_state_v0_6.cpp
    native/free_world_light_transport_state_v0_6.cpp
    "${DEEP_V05}/free_world_deep_scene_binding_v0_5.cpp"
    "${DEEP_V04}/free_world_deep_scene_contribution_v0_4.cpp"
    "${FREE_WORLD_V02}/free_world_pixel_resolve_2d_v0_2.cpp"
    "${APP_CPP}/truthraw_sha256_v0_69.cpp"
)

target_include_directories(test_free_world_light_transport_state_v0_6 PRIVATE
    native
    "${DEEP_V05}"
    "${DEEP_V04}"
    "${FREE_WORLD_V02}"
    "${APP_CPP}"
)

if(MSVC)
  target_compile_options(test_free_world_light_transport_state_v0_6 PRIVATE /W4 /WX)
else()
  target_compile_options(test_free_world_light_transport_state_v0_6 PRIVATE
      -Wall -Wextra -Wpedantic -Werror -ffp-contract=off -fno-fast-math)
endif()

enable_testing()
add_test(NAME free_world_light_transport_state_v0_6
         COMMAND test_free_world_light_transport_state_v0_6)
