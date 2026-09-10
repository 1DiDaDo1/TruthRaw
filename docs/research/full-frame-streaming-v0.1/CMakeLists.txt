cmake_minimum_required(VERSION 3.16)
project(truthraw_full_frame_streaming_v0_1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT DEFINED TRUTHRAW_REPO_ROOT)
  get_filename_component(TRUTHRAW_REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
endif()
set(V47I "${TRUTHRAW_REPO_ROOT}/canonical/reconstruction/v4.7i/native")

add_executable(test_full_frame_streaming_v0_1
  native/full_frame_streaming_v0_1.cpp
  native/full_frame_streaming_v0_1_plan.cpp
  native/full_frame_streaming_v0_1_pass1.cpp
  native/full_frame_streaming_v0_1_pass2.cpp
  tests/test_full_frame_streaming_v0_1.cpp
  "${V47I}/src/core.cpp")
target_include_directories(test_full_frame_streaming_v0_1 PRIVATE
  native
  "${V47I}/include")
target_compile_options(test_full_frame_streaming_v0_1 PRIVATE -Wall -Wextra -Werror)
