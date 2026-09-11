cmake_minimum_required(VERSION 3.22.1)
project(truthraw_ui_preview_bridge LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

get_filename_component(REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../../../../.." ABSOLUTE)
set(RESEARCH_ROOT "${REPO_ROOT}/docs/research")
set(TILE_DNG "${RESEARCH_ROOT}/tile-native-dng-source-v0.1/native")
set(STREAMING "${RESEARCH_ROOT}/full-frame-streaming-v0.1/native")
set(V47I_NATIVE "${REPO_ROOT}/canonical/reconstruction/v4.7i/native")

add_library(truthraw_ui_preview_bridge SHARED
    native_tile_preview_bridge.cpp
    ${TILE_DNG}/tile_native_dng_source_v0_1.cpp
    ${TILE_DNG}/tile_native_dng_source_v0_1_bind.cpp
    ${TILE_DNG}/tile_native_dng_source_v0_1_read.cpp)

target_include_directories(truthraw_ui_preview_bridge PRIVATE
    ${TILE_DNG}
    ${STREAMING}
    ${V47I_NATIVE}/include)

target_compile_options(truthraw_ui_preview_bridge PRIVATE -Wall -Wextra -Werror)
