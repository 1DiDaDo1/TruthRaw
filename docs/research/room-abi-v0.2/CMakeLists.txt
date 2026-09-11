cmake_minimum_required(VERSION 3.16)
project(truthraw_room_abi_v0_2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(RESEARCH_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/..")
set(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../..")
set(TILE_DNG_DIR "${RESEARCH_ROOT}/tile-native-dng-source-v0.1")

add_library(room_abi_v0_2
    native/room_abi_v0_2.cpp
    ${RESEARCH_ROOT}/building-runtime-v0.1/native/building_runtime_v0_1.cpp
    ${RESEARCH_ROOT}/technical-backplane-v0.1/native/technical_backplane_v0_1.cpp
    ${RESEARCH_ROOT}/local-illumination-room-v0.2/native/illumination_room_v0_2.cpp
    ${RESEARCH_ROOT}/counterfactual-illumination-capture-manifold-v1/native/cicm_v1.cpp
    ${RESEARCH_ROOT}/local-illumination-room-capsule-v0.1/native/room_capsule_v0_1.cpp
    ${RESEARCH_ROOT}/room-abi-v0.1/native/room_abi_v0_1.cpp
    ${RESEARCH_ROOT}/manifold-conditioning-v1/native/manifold_conditioning_v1.cpp
)

target_include_directories(room_abi_v0_2 PUBLIC
    native
    ${RESEARCH_ROOT}/building-runtime-v0.1/native
    ${RESEARCH_ROOT}/full-frame-streaming-v0.1/native
    ${RESEARCH_ROOT}/technical-backplane-v0.1/native
    ${RESEARCH_ROOT}/local-illumination-room-v0.2/native
    ${RESEARCH_ROOT}/counterfactual-illumination-capture-manifold-v1/native
    ${RESEARCH_ROOT}/local-illumination-room-capsule-v0.1/native
    ${RESEARCH_ROOT}/room-abi-v0.1/native
    ${RESEARCH_ROOT}/manifold-conditioning-v1/native
    ${REPO_ROOT}/canonical/reconstruction/v4.7i/native/include
)

add_library(tile_native_dng_source_v0_1_for_room_abi STATIC
    ${TILE_DNG_DIR}/native/tile_native_dng_source_v0_1.cpp
    ${TILE_DNG_DIR}/native/tile_native_dng_source_v0_1_bind.cpp
    ${TILE_DNG_DIR}/native/tile_native_dng_source_v0_1_read.cpp
)
target_include_directories(tile_native_dng_source_v0_1_for_room_abi PUBLIC
    ${TILE_DNG_DIR}/native
    ${RESEARCH_ROOT}/full-frame-streaming-v0.1/native
    ${REPO_ROOT}/canonical/reconstruction/v4.7i/native/include
)
target_compile_options(tile_native_dng_source_v0_1_for_room_abi PRIVATE -Wall -Wextra -Werror)

add_executable(test_room_abi_v0_2 tests/test_room_abi_v0_2.cpp)
target_link_libraries(test_room_abi_v0_2 PRIVATE room_abi_v0_2)

add_executable(test_room_abi_v0_2_tile_source tests/test_room_abi_v0_2_tile_source.cpp)
target_link_libraries(test_room_abi_v0_2_tile_source PRIVATE
    room_abi_v0_2
    tile_native_dng_source_v0_1_for_room_abi)
