cmake_minimum_required(VERSION 3.16)
project(truthraw_dng_compatibility_projection_v0_2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(TRUTHRAW_DNG_PROJECTION_V02_ENABLE_SANITIZERS "Enable ASan/UBSan" OFF)

if(NOT DEFINED TRUTHRAW_REPO_ROOT)
  get_filename_component(TRUTHRAW_REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
endif()

set(V47I "${TRUTHRAW_REPO_ROOT}/canonical/reconstruction/v4.7i/native")
set(STREAMING "${TRUTHRAW_REPO_ROOT}/docs/research/full-frame-streaming-v0.1/native")
set(DIGEST "${TRUTHRAW_REPO_ROOT}/docs/research/scientific-master-digest-v0.1/native")
set(BINDING "${TRUTHRAW_REPO_ROOT}/docs/research/scientific-preview-source-binding-v0.1/native")
set(BACKPLANE "${TRUTHRAW_REPO_ROOT}/docs/research/technical-backplane-v0.1/native")
set(TILE_DNG "${TRUTHRAW_REPO_ROOT}/docs/research/tile-native-dng-source-v0.1/native")
set(DNG_V01 "${TRUTHRAW_REPO_ROOT}/docs/research/dng-compatibility-projection-v0.1/native")

add_executable(test_dng_compatibility_projection_v0_2
    tests/test_dng_compatibility_projection_v0_2.cpp
    native/dng_compatibility_projection_v0_2.cpp
    "${DNG_V01}/dng_compatibility_projection_v0_1.cpp"
    "${DIGEST}/scientific_master_digest_v0_1.cpp"
    "${V47I}/src/core.cpp"
)

target_include_directories(test_dng_compatibility_projection_v0_2 PRIVATE
    native
    "${DNG_V01}"
    "${V47I}/include"
    "${STREAMING}"
    "${DIGEST}"
    "${BINDING}"
    "${BACKPLANE}"
    "${TILE_DNG}"
)

if(MSVC)
  target_compile_options(test_dng_compatibility_projection_v0_2 PRIVATE /W4 /WX)
else()
  target_compile_options(test_dng_compatibility_projection_v0_2 PRIVATE
      -Wall -Wextra -Wpedantic -Werror)
endif()

# canonical v4.7i is byte-frozen. Keep the known compact-loop indentation
# diagnostic non-fatal for that one historical translation unit only; the new
# v0.2 writer/wrapper and its test remain under -Werror.
if(NOT MSVC)
  set_source_files_properties(
      "${V47I}/src/core.cpp"
      PROPERTIES COMPILE_OPTIONS "-Wno-error=misleading-indentation")
endif()

if(TRUTHRAW_DNG_PROJECTION_V02_ENABLE_SANITIZERS AND NOT MSVC)
  target_compile_options(test_dng_compatibility_projection_v0_2 PRIVATE
      -fsanitize=address,undefined -fno-omit-frame-pointer)
  target_link_options(test_dng_compatibility_projection_v0_2 PRIVATE
      -fsanitize=address,undefined -fno-omit-frame-pointer)
endif()

enable_testing()
add_test(NAME dng_compatibility_projection_v0_2
         COMMAND test_dng_compatibility_projection_v0_2)
