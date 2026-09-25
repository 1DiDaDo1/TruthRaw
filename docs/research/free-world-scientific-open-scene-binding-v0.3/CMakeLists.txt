cmake_minimum_required(VERSION 3.16)
project(free_world_scientific_open_scene_binding_v0_3 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

get_filename_component(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
set(APP_CPP "${REPO_ROOT}/suite_android/app/src/main/cpp")
set(V47I "${REPO_ROOT}/canonical/reconstruction/v4.7i/native")
set(STREAMING "${REPO_ROOT}/docs/research/full-frame-streaming-v0.1/native")
set(FLOAT_DNG "${REPO_ROOT}/docs/research/scientific-master-linear-dng-projection-v0.1/native")
set(BACKPLANE "${REPO_ROOT}/docs/research/technical-backplane-v0.1/native")
set(TRUTHRANGE "${REPO_ROOT}/docs/research/truthrange-latent-binding-v0.2/native")
set(FREE_WORLD_V02 "${REPO_ROOT}/docs/research/free-world-pixel-resolve-2d-v0.2/native")

add_executable(test_free_world_scientific_open_scene_binding_v0_3
    tests/test_free_world_scientific_open_scene_binding_v0_3.cpp
    native/free_world_scientific_open_scene_binding_v0_3.cpp
    "${FREE_WORLD_V02}/free_world_pixel_resolve_2d_v0_2.cpp"
    "${APP_CPP}/open_scene_field_v0_85.cpp"
    "${APP_CPP}/truthraw_sha256_v0_69.cpp"
)

target_include_directories(test_free_world_scientific_open_scene_binding_v0_3 PRIVATE
    native
    "${FREE_WORLD_V02}"
    "${APP_CPP}"
    "${V47I}/include"
    "${STREAMING}"
    "${FLOAT_DNG}"
    "${BACKPLANE}"
    "${TRUTHRANGE}"
)

if(MSVC)
  target_compile_options(test_free_world_scientific_open_scene_binding_v0_3 PRIVATE /W4 /WX)
else()
  target_compile_options(test_free_world_scientific_open_scene_binding_v0_3 PRIVATE
      -Wall -Wextra -Wpedantic -Werror -ffp-contract=off -fno-fast-math)
endif()

enable_testing()
add_test(NAME free_world_scientific_open_scene_binding_v0_3
         COMMAND test_free_world_scientific_open_scene_binding_v0_3)
