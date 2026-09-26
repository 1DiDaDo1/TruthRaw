cmake_minimum_required(VERSION 3.16)
project(drawnegative_v0_1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

get_filename_component(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
set(DN "${CMAKE_CURRENT_SOURCE_DIR}/native")
set(TN "${REPO_ROOT}/docs/research/truthnegative-continuous-v0.5/native")
set(FW "${REPO_ROOT}/docs/research/free-world-pixel-resolve-2d-v0.2/native")
set(OSF "${REPO_ROOT}/docs/research/open-scene-field-v0.85/native")
set(STREAMING "${REPO_ROOT}/docs/research/full-frame-streaming-v0.1/native")
set(V47I "${REPO_ROOT}/canonical/reconstruction/v4.7i/native")
set(APP "${REPO_ROOT}/suite_android/app/src/main/cpp")
set(STREAMING "${REPO_ROOT}/docs/research/full-frame-streaming-v0.1/native")
set(V47I "${REPO_ROOT}/canonical/reconstruction/v4.7i/native")

add_executable(test_drawnegative_v0_1
    tests/test_drawnegative_v0_1.cpp
    native/drawnegative_v0_1.cpp
    "${TN}/truthnegative_continuous_v0_5.cpp"
    "${FW}/free_world_pixel_resolve_2d_v0_2.cpp"
    "${APP}/open_scene_channel_authority_v0_78.cpp"
    "${APP}/open_scene_field_v0_85.cpp"
    "${APP}/truthraw_sha256_v0_69.cpp"
)

target_include_directories(test_drawnegative_v0_1 PRIVATE
    "${DN}" "${TN}" "${FW}" "${OSF}" "${APP}"
    "${STREAMING}" "${V47I}/include")

if(MSVC)
  target_compile_options(test_drawnegative_v0_1 PRIVATE /W4 /WX)
else()
  target_compile_options(test_drawnegative_v0_1 PRIVATE
    -Wall -Wextra -Wpedantic -Werror -ffp-contract=off -fno-fast-math)
endif()

enable_testing()
add_test(NAME drawnegative_v0_1 COMMAND test_drawnegative_v0_1)
