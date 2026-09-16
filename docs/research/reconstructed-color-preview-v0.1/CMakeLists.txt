cmake_minimum_required(VERSION 3.16)
project(truthraw_reconstructed_color_preview_v0_1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT DEFINED TRUTHRAW_REPO_ROOT)
  get_filename_component(TRUTHRAW_REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
endif()

set(V47I "${TRUTHRAW_REPO_ROOT}/canonical/reconstruction/v4.7i/native")
set(STREAMING "${TRUTHRAW_REPO_ROOT}/docs/research/full-frame-streaming-v0.1/native")

add_library(truthraw_v47i_preview_frozen_core OBJECT "${V47I}/src/core.cpp")
target_include_directories(truthraw_v47i_preview_frozen_core PRIVATE "${V47I}/include")
target_compile_options(truthraw_v47i_preview_frozen_core PRIVATE -Wall -Wextra -Werror)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  target_compile_options(truthraw_v47i_preview_frozen_core PRIVATE -Wno-error=misleading-indentation)
endif()

add_executable(test_reconstructed_color_preview_v0_1
  native/bounded_srgb_preview_sink_v0_1.cpp
  "${STREAMING}/full_frame_streaming_v0_1.cpp"
  "${STREAMING}/full_frame_streaming_v0_1_plan.cpp"
  "${STREAMING}/full_frame_streaming_v0_1_pass1.cpp"
  "${STREAMING}/full_frame_streaming_v0_1_pass2.cpp"
  tests/test_bounded_srgb_preview_sink_v0_1.cpp
  $<TARGET_OBJECTS:truthraw_v47i_preview_frozen_core>)

target_include_directories(test_reconstructed_color_preview_v0_1 PRIVATE
  native
  "${STREAMING}"
  "${V47I}/include")

target_compile_options(test_reconstructed_color_preview_v0_1 PRIVATE -Wall -Wextra -Werror)
