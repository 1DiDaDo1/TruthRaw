cmake_minimum_required(VERSION 3.16)
project(truthraw_dng_compatibility_projection_v0_1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(TRUTHRAW_DNG_PROJECTION_ENABLE_SANITIZERS "Enable ASan/UBSan" OFF)

if(NOT DEFINED TRUTHRAW_REPO_ROOT)
  get_filename_component(TRUTHRAW_REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
endif()

set(V47I "${TRUTHRAW_REPO_ROOT}/canonical/reconstruction/v4.7i/native")
set(STREAMING "${TRUTHRAW_REPO_ROOT}/docs/research/full-frame-streaming-v0.1/native")
set(DIGEST "${TRUTHRAW_REPO_ROOT}/docs/research/scientific-master-digest-v0.1/native")
set(BINDING "${TRUTHRAW_REPO_ROOT}/docs/research/scientific-preview-source-binding-v0.1/native")
set(BACKPLANE "${TRUTHRAW_REPO_ROOT}/docs/research/technical-backplane-v0.1/native")
set(TILE_DNG "${TRUTHRAW_REPO_ROOT}/docs/research/tile-native-dng-source-v0.1/native")

add_executable(test_dng_compatibility_projection_v0_1
    tests/test_dng_compatibility_projection_v0_1.cpp
    native/dng_compatibility_projection_v0_1.cpp
    "${DIGEST}/scientific_master_digest_v0_1.cpp"
)

target_include_directories(test_dng_compatibility_projection_v0_1 PRIVATE
    native
    "${V47I}/include"
    "${STREAMING}"
    "${DIGEST}"
    "${BINDING}"
    "${BACKPLANE}"
    "${TILE_DNG}"
)

if(MSVC)
  target_compile_options(test_dng_compatibility_projection_v0_1 PRIVATE /W4 /WX)
else()
  target_compile_options(test_dng_compatibility_projection_v0_1 PRIVATE
      -Wall -Wextra -Wpedantic -Werror)
endif()

if(TRUTHRAW_DNG_PROJECTION_ENABLE_SANITIZERS AND NOT MSVC)
  target_compile_options(test_dng_compatibility_projection_v0_1 PRIVATE
      -fsanitize=address,undefined -fno-omit-frame-pointer)
  target_link_options(test_dng_compatibility_projection_v0_1 PRIVATE
      -fsanitize=address,undefined -fno-omit-frame-pointer)
endif()

enable_testing()
add_test(NAME dng_compatibility_projection_v0_1
         COMMAND test_dng_compatibility_projection_v0_1)
