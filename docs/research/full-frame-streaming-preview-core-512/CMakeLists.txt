cmake_minimum_required(VERSION 3.16)
project(truthraw_full_frame_streaming_preview_core_512 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(TRUTHRAW_PREVIEW512_ENABLE_SANITIZERS "Enable ASan/UBSan" OFF)

if(NOT DEFINED TRUTHRAW_REPO_ROOT)
  get_filename_component(TRUTHRAW_REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
endif()

set(V47I "${TRUTHRAW_REPO_ROOT}/canonical/reconstruction/v4.7i/native")
set(STREAMING "${TRUTHRAW_REPO_ROOT}/docs/research/full-frame-streaming-v0.1/native")
set(STREAMING_TESTS "${TRUTHRAW_REPO_ROOT}/docs/research/full-frame-streaming-v0.1/tests")
set(PREVIEW "${TRUTHRAW_REPO_ROOT}/docs/research/reconstructed-color-preview-v0.1/native")

add_library(truthraw_v47i_frozen_core_preview512 OBJECT "${V47I}/src/core.cpp")
target_include_directories(truthraw_v47i_frozen_core_preview512 PRIVATE "${V47I}/include")
target_compile_options(truthraw_v47i_frozen_core_preview512 PRIVATE -Wall -Wextra -Werror)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  target_compile_options(truthraw_v47i_frozen_core_preview512 PRIVATE -Wno-error=misleading-indentation)
endif()

add_executable(test_full_frame_streaming_preview_core_512
    tests/test_full_frame_streaming_preview_core_512.cpp
    "${STREAMING}/full_frame_streaming_v0_1.cpp"
    "${STREAMING}/full_frame_streaming_v0_1_plan.cpp"
    "${STREAMING}/full_frame_streaming_v0_1_pass1.cpp"
    "${STREAMING}/full_frame_streaming_v0_1_pass2.cpp"
    "${PREVIEW}/bounded_srgb_preview_sink_v0_1.cpp"
    $<TARGET_OBJECTS:truthraw_v47i_frozen_core_preview512>
)

target_include_directories(test_full_frame_streaming_preview_core_512 PRIVATE
    tests
    "${STREAMING}"
    "${STREAMING_TESTS}"
    "${PREVIEW}"
    "${V47I}/include"
)

if(MSVC)
  target_compile_options(test_full_frame_streaming_preview_core_512 PRIVATE /W4 /WX)
else()
  target_compile_options(test_full_frame_streaming_preview_core_512 PRIVATE
      -Wall -Wextra -Wpedantic -Werror)
endif()

if(TRUTHRAW_PREVIEW512_ENABLE_SANITIZERS AND NOT MSVC)
  target_compile_options(test_full_frame_streaming_preview_core_512 PRIVATE
      -fsanitize=address,undefined -fno-omit-frame-pointer)
  target_link_options(test_full_frame_streaming_preview_core_512 PRIVATE
      -fsanitize=address,undefined -fno-omit-frame-pointer)
endif()

enable_testing()
add_test(NAME full_frame_streaming_preview_core_512
         COMMAND test_full_frame_streaming_preview_core_512)
