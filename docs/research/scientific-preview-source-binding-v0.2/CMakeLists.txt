cmake_minimum_required(VERSION 3.16)
project(truthraw_scientific_preview_source_binding_v0_2 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

get_filename_component(TRUTHRAW_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
set(V01_DIR "${TRUTHRAW_ROOT}/docs/research/scientific-preview-source-binding-v0.1")
set(TILE_DIR "${TRUTHRAW_ROOT}/docs/research/tile-native-dng-source-v0.1")
set(STREAM_DIR "${TRUTHRAW_ROOT}/docs/research/full-frame-streaming-v0.1")
set(BACKPLANE_DIR "${TRUTHRAW_ROOT}/docs/research/technical-backplane-v0.1")
set(V47I_INC "${TRUTHRAW_ROOT}/canonical/reconstruction/v4.7i/native/include")

add_executable(test_scientific_preview_source_binding_v0_2
    native/scientific_preview_source_binding_v0_2.cpp
    tests/test_scientific_preview_source_binding_v0_2.cpp
    "${V01_DIR}/native/scientific_preview_source_binding_v0_1.cpp"
    "${BACKPLANE_DIR}/native/technical_backplane_v0_1.cpp")

target_include_directories(test_scientific_preview_source_binding_v0_2 PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/native"
    "${V01_DIR}/native"
    "${TILE_DIR}/native"
    "${STREAM_DIR}/native"
    "${BACKPLANE_DIR}/native"
    "${V47I_INC}")

target_compile_options(test_scientific_preview_source_binding_v0_2 PRIVATE -Wall -Wextra -Werror)
