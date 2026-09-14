cmake_minimum_required(VERSION 3.16)
project(truthraw_full_frame_streaming_v0_2_multiworker LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT DEFINED TRUTHRAW_REPO_ROOT)
  get_filename_component(TRUTHRAW_REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
endif()

set(V47I "${TRUTHRAW_REPO_ROOT}/canonical/reconstruction/v4.7i/native")
set(STREAM_V01 "${TRUTHRAW_REPO_ROOT}/docs/research/full-frame-streaming-v0.1")

find_package(Threads REQUIRED)

add_library(truthraw_v47i_frozen_core_v02 OBJECT
  "${V47I}/src/core.cpp")
target_include_directories(truthraw_v47i_frozen_core_v02 PRIVATE
  "${V47I}/include")
target_compile_options(truthraw_v47i_frozen_core_v02 PRIVATE
  -Wall -Wextra -Werror)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  target_compile_options(truthraw_v47i_frozen_core_v02 PRIVATE
    -Wno-error=misleading-indentation)
endif()

add_executable(test_full_frame_streaming_v0_2_multiworker
  "${STREAM_V01}/native/full_frame_streaming_v0_1.cpp"
  "${STREAM_V01}/native/full_frame_streaming_v0_1_plan.cpp"
  "${STREAM_V01}/native/full_frame_streaming_v0_1_pass1.cpp"
  "${STREAM_V01}/native/full_frame_streaming_v0_1_pass2.cpp"
  test_full_frame_streaming_v0_2_multiworker.cpp
  $<TARGET_OBJECTS:truthraw_v47i_frozen_core_v02>)

target_include_directories(test_full_frame_streaming_v0_2_multiworker PRIVATE
  "${STREAM_V01}/native"
  "${STREAM_V01}/tests"
  "${V47I}/include")

target_compile_options(test_full_frame_streaming_v0_2_multiworker PRIVATE
  -Wall -Wextra -Werror)
target_link_libraries(test_full_frame_streaming_v0_2_multiworker PRIVATE Threads::Threads)

enable_testing()
add_test(NAME full_frame_streaming_v0_2_multiworker
  COMMAND test_full_frame_streaming_v0_2_multiworker)
