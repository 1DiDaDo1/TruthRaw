cmake_minimum_required(VERSION 3.16)
project(truthraw_xyz_d50_uncertainty_v0_7 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(TRUTHRAW_REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../..")
set(TRUTHRAW_V06_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../camera-rgb-covariance-v0.6")

foreach(req
    "${TRUTHRAW_V06_DIR}/CMakeLists.txt"
    "${TRUTHRAW_V06_DIR}/native/camera_rgb_covariance_v0_6.h"
    "${TRUTHRAW_V06_DIR}/native/camera_rgb_covariance_v0_6.cpp")
  if(NOT EXISTS "${req}")
    message(FATAL_ERROR "Required closed TruthRaw v0.6 dependency missing: ${req}")
  endif()
endforeach()

# Build the real repository v0.6 target (and its dependency checks) rather than
# compiling v0.7 against a copied or synthetic interface.
add_subdirectory(
    "${TRUTHRAW_V06_DIR}"
    "${CMAKE_BINARY_DIR}/camera_rgb_covariance_v06")

add_library(xyz_d50_uncertainty_v0_7
    native/xyz_d50_uncertainty_v0_7.cpp)

target_include_directories(xyz_d50_uncertainty_v0_7 PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/native"
    "${TRUTHRAW_V06_DIR}/native")

target_link_libraries(xyz_d50_uncertainty_v0_7 PUBLIC camera_rgb_covariance_v0_6)

add_executable(test_xyz_d50_uncertainty_v0_7
    tests/test_xyz_d50_uncertainty_v0_7.cpp)
target_link_libraries(test_xyz_d50_uncertainty_v0_7 PRIVATE xyz_d50_uncertainty_v0_7)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  foreach(t xyz_d50_uncertainty_v0_7 test_xyz_d50_uncertainty_v0_7)
    target_compile_options(${t} PRIVATE -Wall -Wextra -Wpedantic -Werror)
  endforeach()
endif()

enable_testing()
add_test(NAME xyz_d50_uncertainty_v0_7 COMMAND test_xyz_d50_uncertainty_v0_7)
