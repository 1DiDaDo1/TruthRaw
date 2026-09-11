cmake_minimum_required(VERSION 3.16)
project(truthraw_dng_source_color_binding_v0_1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

get_filename_component(TRUTHRAW_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
set(TILE_DIR "${TRUTHRAW_ROOT}/docs/research/tile-native-dng-source-v0.1")
set(BACKPLANE_DIR "${TRUTHRAW_ROOT}/docs/research/technical-backplane-v0.1")
set(SOURCE_BIND_DIR "${TRUTHRAW_ROOT}/docs/research/scientific-preview-source-binding-v0.1")
set(STREAM_DIR "${TRUTHRAW_ROOT}/docs/research/full-frame-streaming-v0.1")
set(V47I_INC "${TRUTHRAW_ROOT}/canonical/reconstruction/v4.7i/native/include")

add_executable(test_dng_source_color_binding_v0_1
    native/dng_source_color_binding_v0_1.cpp
    tests/test_dng_source_color_binding_v0_1.cpp
    "${SOURCE_BIND_DIR}/native/scientific_preview_source_binding_v0_1.cpp"
    "${BACKPLANE_DIR}/native/technical_backplane_v0_1.cpp")

target_include_directories(test_dng_source_color_binding_v0_1 PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/native"
    "${SOURCE_BIND_DIR}/native"
    "${TILE_DIR}/native"
    "${STREAM_DIR}/native"
    "${BACKPLANE_DIR}/native"
    "${V47I_INC}")

target_compile_options(test_dng_source_color_binding_v0_1 PRIVATE -Wall -Wextra -Werror)
