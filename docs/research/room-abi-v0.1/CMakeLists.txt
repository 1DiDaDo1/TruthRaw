cmake_minimum_required(VERSION 3.16)
project(truthraw_room_abi_v0_1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../..")
add_executable(test_room_abi_v0_1
    native/room_abi_v0_1.cpp
    tests/test_room_abi_v0_1.cpp
    ${REPO_ROOT}/docs/research/building-runtime-v0.1/native/building_runtime_v0_1.cpp
    ${REPO_ROOT}/docs/research/manifold-conditioning-v1/native/manifold_conditioning_v1.cpp
    ${REPO_ROOT}/docs/research/local-illumination-room-capsule-v0.1/native/room_capsule_v0_1.cpp
)
target_include_directories(test_room_abi_v0_1 PRIVATE
    native
    ${REPO_ROOT}/docs/research/building-runtime-v0.1/native
    ${REPO_ROOT}/docs/research/manifold-conditioning-v1/native
    ${REPO_ROOT}/docs/research/local-illumination-room-capsule-v0.1/native
    ${REPO_ROOT}/canonical/reconstruction/v4.7i/native/include
)
target_compile_options(test_room_abi_v0_1 PRIVATE -Wall -Wextra -Wpedantic -Werror)
