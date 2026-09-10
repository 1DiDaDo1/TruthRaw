cmake_minimum_required(VERSION 3.16)
project(truthraw_full_frame_streaming_v0_1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT DEFINED TRUTHRAW_REPO_ROOT)
  get_filename_component(TRUTHRAW_REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
endif()
set(V47I "${TRUTHRAW_REPO_ROOT}/canonical/reconstruction/v4.7i/native")

# Keep the new adapter/tests on strict -Werror. The frozen canonical v4.7i
# source is compiled as a separate object so a pre-existing Clang-only
# misleading-indentation warning remains visible but does not require changing
# byte-frozen canonical source merely to exercise cross-compiler equivalence.
add_library(truthraw_v47i_frozen_core OBJECT "${V47I}/src/core.cpp")
target_include_directories(truthraw_v47i_frozen_core PRIVATE "${V47I}/include")
target_compile_options(truthraw_v47i_frozen_core PRIVATE -Wall -Wextra -Werror)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  target_compile_options(truthraw_v47i_frozen_core PRIVATE -Wno-error=misleading-indentation)
endif()

add_executable(test_full_frame_streaming_v0_1
  native/full_frame_streaming_v0_1.cpp
  native/full_frame_streaming_v0_1_plan.cpp
  native/full_frame_streaming_v0_1_pass1.cpp
  native/full_frame_streaming_v0_1_pass2.cpp
  tests/test_full_frame_streaming_v0_1.cpp
  $<TARGET_OBJECTS:truthraw_v47i_frozen_core>)
target_include_directories(test_full_frame_streaming_v0_1 PRIVATE
  native
  "${V47I}/include")
target_compile_options(test_full_frame_streaming_v0_1 PRIVATE -Wall -Wextra -Werror)
