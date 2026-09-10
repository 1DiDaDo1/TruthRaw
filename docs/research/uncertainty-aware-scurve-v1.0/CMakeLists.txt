cmake_minimum_required(VERSION 3.16)
project(truthraw_uncertainty_aware_scurve_v1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(V07_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../xyz-d50-uncertainty-v0.7")
set(V06_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../camera-rgb-covariance-v0.6")
foreach(req
    "${V07_DIR}/native/xyz_d50_uncertainty_v0_7.h"
    "${V06_DIR}/native/camera_rgb_covariance_v0_6.h")
  if(NOT EXISTS "${req}")
    message(FATAL_ERROR "Required closed TruthRaw uncertainty dependency missing: ${req}")
  endif()
endforeach()

add_library(uncertainty_aware_scurve_v1
    native/uncertainty_aware_scurve_v1.cpp
    native/xyz_uncertainty_adapter_v1.cpp)
target_include_directories(uncertainty_aware_scurve_v1 PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/native"
    "${V07_DIR}/native"
    "${V06_DIR}/native")

add_executable(test_uncertainty_aware_scurve_v1 tests/test_uncertainty_aware_scurve_v1.cpp)
target_link_libraries(test_uncertainty_aware_scurve_v1 PRIVATE uncertainty_aware_scurve_v1)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  foreach(t uncertainty_aware_scurve_v1 test_uncertainty_aware_scurve_v1)
    target_compile_options(${t} PRIVATE -Wall -Wextra -Wpedantic -Werror)
  endforeach()
endif()

enable_testing()
add_test(NAME uncertainty_aware_scurve_v1 COMMAND test_uncertainty_aware_scurve_v1)
