cmake_minimum_required(VERSION 3.16)
project(truthraw_illumination_room_v0_2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

get_filename_component(TRUTHRAW_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)

add_executable(test_illumination_room_v0_2
    native/illumination_room_v0_2.cpp
    tests/test_illumination_room_v0_2.cpp
    ${TRUTHRAW_ROOT}/docs/research/local-illumination-room-capsule-v0.1/native/room_capsule_v0_1.cpp
    ${TRUTHRAW_ROOT}/docs/research/counterfactual-illumination-capture-manifold-v1/native/cicm_v1.cpp
    ${TRUTHRAW_ROOT}/docs/research/technical-backplane-v0.1/native/technical_backplane_v0_1.cpp
    ${TRUTHRAW_ROOT}/docs/research/room-abi-v0.1/native/room_abi_v0_1.cpp
    ${TRUTHRAW_ROOT}/docs/research/manifold-conditioning-v1/native/manifold_conditioning_v1.cpp
)

target_include_directories(test_illumination_room_v0_2 PRIVATE
    native
    ${TRUTHRAW_ROOT}/docs/research/local-illumination-room-capsule-v0.1/native
    ${TRUTHRAW_ROOT}/docs/research/counterfactual-illumination-capture-manifold-v1/native
    ${TRUTHRAW_ROOT}/docs/research/technical-backplane-v0.1/native
    ${TRUTHRAW_ROOT}/docs/research/room-abi-v0.1/native
    ${TRUTHRAW_ROOT}/docs/research/building-runtime-v0.1/native
    ${TRUTHRAW_ROOT}/docs/research/manifold-conditioning-v1/native
    ${TRUTHRAW_ROOT}/canonical/reconstruction/v4.7i/native/include
)

target_compile_options(test_illumination_room_v0_2 PRIVATE -Wall -Wextra -Wpedantic -Werror)

enable_testing()
add_test(NAME illumination_room_v0_2 COMMAND test_illumination_room_v0_2)
