cmake_minimum_required(VERSION 3.20)
project(truthraw_linear_dng_compatibility_projection_v0_1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

get_filename_component(REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
set(V47I_NATIVE "${REPO_ROOT}/canonical/reconstruction/v4.7i/native")
set(STREAMING "${REPO_ROOT}/docs/research/full-frame-streaming-v0.1/native")
set(SOURCE_BINDING "${REPO_ROOT}/docs/research/scientific-preview-source-binding-v0.1/native")
set(TILE_DNG "${REPO_ROOT}/docs/research/tile-native-dng-source-v0.1/native")

add_executable(linear_dng_projection_v0_1_test
    native/linear_dng_compatibility_projection_v0_1.cpp
    tests/test_linear_dng_compatibility_projection_v0_1.cpp
    ${V47I_NATIVE}/src/core.cpp)

target_include_directories(linear_dng_projection_v0_1_test PRIVATE
    native
    ${V47I_NATIVE}/include
    ${STREAMING}
    ${SOURCE_BINDING}
    ${TILE_DNG})

target_compile_options(linear_dng_projection_v0_1_test PRIVATE -Wall -Wextra -Werror)

set_source_files_properties(
    ${V47I_NATIVE}/src/core.cpp
    PROPERTIES COMPILE_OPTIONS "-Wno-error=misleading-indentation")

enable_testing()
add_test(NAME linear_dng_projection_v0_1 COMMAND linear_dng_projection_v0_1_test)
