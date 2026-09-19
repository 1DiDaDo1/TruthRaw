cmake_minimum_required(VERSION 3.20)
project(truthraw_multivendor_raw_source_adapter_v0_1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

get_filename_component(REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
set(MODULE_ROOT "${CMAKE_CURRENT_LIST_DIR}")
set(STREAMING "${REPO_ROOT}/docs/research/full-frame-streaming-v0.1/native")
set(TILE_DNG "${REPO_ROOT}/docs/research/tile-native-dng-source-v0.1/native")
set(V47I "${REPO_ROOT}/canonical/reconstruction/v4.7i/native")

add_executable(test_multivendor_raw_source_adapter_v0_1
    native/multivendor_raw_source_adapter_v0_1.cpp
    native/nikon_nef_uncompressed_adapter_v0_1.cpp
    tests/test_multivendor_raw_source_adapter_v0_1.cpp
    ${TILE_DNG}/tile_native_dng_source_v0_1.cpp
    ${TILE_DNG}/tile_native_dng_source_v0_1_bind.cpp
    ${TILE_DNG}/tile_native_dng_source_v0_1_read.cpp
)

target_include_directories(test_multivendor_raw_source_adapter_v0_1 PRIVATE
    ${MODULE_ROOT}/native
    ${STREAMING}
    ${TILE_DNG}
    ${V47I}/include
)

target_compile_options(test_multivendor_raw_source_adapter_v0_1 PRIVATE
    -Wall -Wextra -Werror
)

add_executable(test_nikon_nef_uncompressed_adapter_v0_1
    native/multivendor_raw_source_adapter_v0_1.cpp
    native/nikon_nef_uncompressed_adapter_v0_1.cpp
    tests/test_nikon_nef_uncompressed_adapter_v0_1.cpp
    ${TILE_DNG}/tile_native_dng_source_v0_1.cpp
    ${TILE_DNG}/tile_native_dng_source_v0_1_bind.cpp
    ${TILE_DNG}/tile_native_dng_source_v0_1_read.cpp
)

target_include_directories(test_nikon_nef_uncompressed_adapter_v0_1 PRIVATE
    ${MODULE_ROOT}/native
    ${STREAMING}
    ${TILE_DNG}
    ${V47I}/include
)

target_compile_options(test_nikon_nef_uncompressed_adapter_v0_1 PRIVATE
    -Wall -Wextra -Werror
)
