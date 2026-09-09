cmake_minimum_required(VERSION 3.16)
project(truthraw_virtual_observation_manifold_v0_9 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(TRUTHRAW_REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../..")
set(TRUTHRAW_V06_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../camera-rgb-covariance-v0.6")
set(TRUTHRAW_V02_NATIVE "${CMAKE_CURRENT_SOURCE_DIR}/../truthrange-latent-binding-v0.2/native")

foreach(req
    "${TRUTHRAW_V06_DIR}/CMakeLists.txt"
    "${TRUTHRAW_V06_DIR}/native/camera_rgb_covariance_v0_6.h"
    "${TRUTHRAW_V02_NATIVE}/truthrange_latent_v0_2.h")
  if(NOT EXISTS "${req}")
    message(FATAL_ERROR "Required closed TruthRaw dependency missing: ${req}")
  endif()
endforeach()

# Build against the real closed v0.6 dependency; v0.6 exposes the real v0.2/v0.3/v4.7i include chain.
add_subdirectory(
    "${TRUTHRAW_V06_DIR}"
    "${CMAKE_BINARY_DIR}/camera_rgb_covariance_v06")

add_library(virtual_observation_manifold_v0_9
    native/virtual_observation_manifold_v0_9.cpp)

target_include_directories(virtual_observation_manifold_v0_9 PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/native"
    "${TRUTHRAW_V02_NATIVE}"
    "${TRUTHRAW_V06_DIR}/native")

target_link_libraries(virtual_observation_manifold_v0_9 PUBLIC camera_rgb_covariance_v0_6)

add_executable(test_virtual_observation_manifold_v0_9
    tests/test_virtual_observation_manifold_v0_9.cpp)
target_link_libraries(test_virtual_observation_manifold_v0_9 PRIVATE virtual_observation_manifold_v0_9)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  foreach(t virtual_observation_manifold_v0_9 test_virtual_observation_manifold_v0_9)
    target_compile_options(${t} PRIVATE -Wall -Wextra -Wpedantic -Werror)
  endforeach()
endif()

enable_testing()
add_test(NAME virtual_observation_manifold_v0_9 COMMAND test_virtual_observation_manifold_v0_9)
