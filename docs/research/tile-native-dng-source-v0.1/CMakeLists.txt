cmake_minimum_required(VERSION 3.16)
project(truthraw_tile_native_dng_source_v0_1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

get_filename_component(TRUTHRAW_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
set(V47I_INC "${TRUTHRAW_ROOT}/canonical/reconstruction/v4.7i/native/include")
set(V47I_SRC "${TRUTHRAW_ROOT}/canonical/reconstruction/v4.7i/native/src/core.cpp")
set(STREAM_DIR "${TRUTHRAW_ROOT}/docs/research/full-frame-streaming-v0.1")

add_library(tile_native_dng_source_v0_1 STATIC
    native/tile_native_dng_source_v0_1.cpp
    native/tile_native_dng_source_v0_1_bind.cpp
    native/tile_native_dng_source_v0_1_read.cpp)
target_include_directories(tile_native_dng_source_v0_1 PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/native"
    "${STREAM_DIR}/native"
    "${V47I_INC}")
target_compile_options(tile_native_dng_source_v0_1 PRIVATE -Wall -Wextra -Werror)

add_executable(test_tile_native_dng_source_v0_1
    tests/test_tile_native_dng_source_v0_1.cpp)
target_link_libraries(test_tile_native_dng_source_v0_1 PRIVATE tile_native_dng_source_v0_1)
target_compile_options(test_tile_native_dng_source_v0_1 PRIVATE -Wall -Wextra -Werror)

# Frozen v4.7i is byte-bound upstream. Clang 18 reports one known historical
# misleading-indentation warning in core.cpp; it is reported but not promoted
# to a new error. New v0.1 code remains -Werror.
add_library(truthraw_v47i_frozen_core OBJECT "${V47I_SRC}")
target_include_directories(truthraw_v47i_frozen_core PRIVATE "${V47I_INC}")
target_compile_options(truthraw_v47i_frozen_core PRIVATE -Wall -Wextra)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(truthraw_v47i_frozen_core PRIVATE -Wno-error=misleading-indentation)
endif()

add_library(full_frame_streaming_v0_1_for_integration STATIC
    "${STREAM_DIR}/native/full_frame_streaming_v0_1.cpp"
    "${STREAM_DIR}/native/full_frame_streaming_v0_1_plan.cpp"
    "${STREAM_DIR}/native/full_frame_streaming_v0_1_pass1.cpp"
    "${STREAM_DIR}/native/full_frame_streaming_v0_1_pass2.cpp"
    $<TARGET_OBJECTS:truthraw_v47i_frozen_core>)
target_include_directories(full_frame_streaming_v0_1_for_integration PUBLIC
    "${STREAM_DIR}/native" "${V47I_INC}")
target_compile_options(full_frame_streaming_v0_1_for_integration PRIVATE -Wall -Wextra -Werror)

add_executable(test_tile_native_dng_streaming_integration_v0_1
    tests/test_tile_native_dng_streaming_integration_v0_1.cpp)
target_include_directories(test_tile_native_dng_streaming_integration_v0_1 PRIVATE
    "${STREAM_DIR}/tests")
target_link_libraries(test_tile_native_dng_streaming_integration_v0_1 PRIVATE
    tile_native_dng_source_v0_1 full_frame_streaming_v0_1_for_integration)
target_compile_options(test_tile_native_dng_streaming_integration_v0_1 PRIVATE -Wall -Wextra -Werror)
