cmake_minimum_required(VERSION 3.16)
project(truthraw_camera_rgb_covariance_v0_6 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(TRUTHRAW_REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../..")
set(TRUTHRANGE_V03_NATIVE "${CMAKE_CURRENT_SOURCE_DIR}/../truthrange-dense-uncertainty-v0.3/native")
set(TRUTHRANGE_V02_NATIVE "${CMAKE_CURRENT_SOURCE_DIR}/../truthrange-latent-binding-v0.2/native")
set(TRUTHRAW_V47I_INCLUDE "${TRUTHRAW_REPO_ROOT}/canonical/reconstruction/v4.7i/native/include")

foreach(req
    "${TRUTHRANGE_V03_NATIVE}/truthrange_dense_uncertainty_v0_3.h"
    "${TRUTHRANGE_V02_NATIVE}/truthrange_latent_v0_2.h"
    "${TRUTHRAW_V47I_INCLUDE}/truthraw/core.h")
  if(NOT EXISTS "${req}")
    message(FATAL_ERROR "Required closed TruthRaw dependency missing: ${req}")
  endif()
endforeach()

add_library(camera_rgb_covariance_v0_6
    native/camera_rgb_covariance_v0_6.cpp
    native/dense_uncertainty_v0_3_adapter_v0_6.cpp)

target_include_directories(camera_rgb_covariance_v0_6 PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/native"
    "${TRUTHRANGE_V03_NATIVE}"
    "${TRUTHRANGE_V02_NATIVE}"
    "${TRUTHRAW_V47I_INCLUDE}")

add_executable(test_camera_rgb_covariance_v0_6
    tests/test_camera_rgb_covariance_v0_6.cpp)
target_link_libraries(test_camera_rgb_covariance_v0_6 PRIVATE camera_rgb_covariance_v0_6)

add_executable(test_dense_uncertainty_v0_3_adapter_v0_6
    tests/test_dense_uncertainty_v0_3_adapter_v0_6.cpp)
target_link_libraries(test_dense_uncertainty_v0_3_adapter_v0_6 PRIVATE camera_rgb_covariance_v0_6)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  foreach(t camera_rgb_covariance_v0_6 test_camera_rgb_covariance_v0_6 test_dense_uncertainty_v0_3_adapter_v0_6)
    target_compile_options(${t} PRIVATE -Wall -Wextra -Wpedantic -Werror)
  endforeach()
endif()

enable_testing()
add_test(NAME camera_rgb_covariance_v0_6 COMMAND test_camera_rgb_covariance_v0_6)
add_test(NAME dense_uncertainty_v0_3_adapter_v0_6 COMMAND test_dense_uncertainty_v0_3_adapter_v0_6)
