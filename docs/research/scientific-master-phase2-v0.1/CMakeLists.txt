cmake_minimum_required(VERSION 3.16)
project(truthraw_scientific_master_phase2_v0_1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(TRUTHRAW_PHASE2_ENABLE_SANITIZERS "Enable Address/UndefinedBehavior sanitizers" OFF)

if(NOT DEFINED TRUTHRAW_REPO_ROOT)
  get_filename_component(TRUTHRAW_REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
endif()
set(V47I "${TRUTHRAW_REPO_ROOT}/canonical/reconstruction/v4.7i/native")
set(STREAMING "${TRUTHRAW_REPO_ROOT}/docs/research/full-frame-streaming-v0.1")
set(DIGEST "${TRUTHRAW_REPO_ROOT}/docs/research/scientific-master-digest-v0.1")

add_library(truthraw_v47i_frozen_core_phase2 OBJECT "${V47I}/src/core.cpp")
target_include_directories(truthraw_v47i_frozen_core_phase2 PRIVATE "${V47I}/include")
target_compile_options(truthraw_v47i_frozen_core_phase2 PRIVATE -Wall -Wextra -Werror)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  target_compile_options(truthraw_v47i_frozen_core_phase2 PRIVATE -Wno-error=misleading-indentation)
endif()

add_executable(test_scientific_master_phase2_v0_1
  native/scientific_master_phase2_v0_1.cpp
  "${DIGEST}/native/scientific_master_digest_v0_1.cpp"
  "${STREAMING}/native/full_frame_streaming_v0_1_plan.cpp"
  "${STREAMING}/native/full_frame_streaming_v0_1_pass1.cpp"
  tests/test_scientific_master_phase2_v0_1.cpp
  $<TARGET_OBJECTS:truthraw_v47i_frozen_core_phase2>)

target_include_directories(test_scientific_master_phase2_v0_1 PRIVATE
  native
  "${DIGEST}/native"
  "${STREAMING}/native"
  "${STREAMING}/tests"
  "${V47I}/include")
target_compile_options(test_scientific_master_phase2_v0_1 PRIVATE -Wall -Wextra -Werror)

if(TRUTHRAW_PHASE2_ENABLE_SANITIZERS AND NOT MSVC)
  target_compile_options(test_scientific_master_phase2_v0_1 PRIVATE
    -fsanitize=address,undefined -fno-omit-frame-pointer)
  target_link_options(test_scientific_master_phase2_v0_1 PRIVATE
    -fsanitize=address,undefined -fno-omit-frame-pointer)
endif()

enable_testing()
add_test(NAME scientific_master_phase2_v0_1 COMMAND test_scientific_master_phase2_v0_1)
