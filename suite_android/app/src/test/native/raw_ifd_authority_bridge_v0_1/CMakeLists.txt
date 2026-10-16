cmake_minimum_required(VERSION 3.20)
project(raw_ifd_authority_bridge_v0_1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

get_filename_component(REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../../../.." ABSOLUTE)
set(BRIDGE_CPP "${REPO_ROOT}/suite_android/app/src/main/cpp")
set(MULTIVENDOR "${REPO_ROOT}/docs/research/multivendor-raw-source-adapter-v0.1/native")
set(TILE_DNG "${REPO_ROOT}/docs/research/tile-native-dng-source-v0.2/native")
set(STREAMING "${REPO_ROOT}/docs/research/full-frame-streaming-v0.1/native")
set(SOURCE_BINDING "${REPO_ROOT}/docs/research/scientific-preview-source-binding-v0.1/native")
set(BACKPLANE "${REPO_ROOT}/docs/research/technical-backplane-v0.1/native")
set(V47I "${REPO_ROOT}/canonical/reconstruction/v4.7i/native/include")

add_executable(test_raw_ifd_authority_bridge_v0_1
    test_raw_ifd_authority_bridge_v0_1.cpp
    "${MULTIVENDOR}/multivendor_raw_source_adapter_v0_1.cpp"
    "${TILE_DNG}/tile_native_dng_source_v0_1.cpp"
    "${TILE_DNG}/tile_native_dng_source_v0_1_bind.cpp"
    "${TILE_DNG}/tile_native_dng_source_v0_1_read.cpp")

target_include_directories(test_raw_ifd_authority_bridge_v0_1 PRIVATE
    "${BRIDGE_CPP}"
    "${MULTIVENDOR}"
    "${TILE_DNG}"
    "${STREAMING}"
    "${SOURCE_BINDING}"
    "${BACKPLANE}"
    "${V47I}")

target_compile_options(test_raw_ifd_authority_bridge_v0_1 PRIVATE
    -Wall -Wextra -Werror)
