cmake_minimum_required(VERSION 3.16)
project(truthraw_scientific_master_linear_dng_projection_v0_1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(TRUTHRAW_SMLDP_V01_ENABLE_SANITIZERS "Enable ASan/UBSan" OFF)

if(NOT DEFINED TRUTHRAW_REPO_ROOT)
  get_filename_component(TRUTHRAW_REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
endif()

set(DIGEST "${TRUTHRAW_REPO_ROOT}/docs/research/scientific-master-digest-v0.1/native")
set(STREAMING "${TRUTHRAW_REPO_ROOT}/docs/research/full-frame-streaming-v0.1")
set(SMSB1 "${TRUTHRAW_REPO_ROOT}/docs/research/scientific-master-streaming-binding-v0.1")
set(SMSB2 "${TRUTHRAW_REPO_ROOT}/docs/research/scientific-master-streaming-binding-v0.2")
set(TRUTHRANGE "${TRUTHRAW_REPO_ROOT}/docs/research/truthrange-latent-binding-v0.2")
set(TILE_DNG "${TRUTHRAW_REPO_ROOT}/docs/research/tile-native-dng-source-v0.1/native")
set(V47I "${TRUTHRAW_REPO_ROOT}/canonical/reconstruction/v4.7i/native")

add_library(truthraw_v47i_frozen_core_smldp OBJECT "${V47I}/src/core.cpp")
target_include_directories(truthraw_v47i_frozen_core_smldp PRIVATE "${V47I}/include")
if(MSVC)
  target_compile_options(truthraw_v47i_frozen_core_smldp PRIVATE /W4 /WX)
else()
  target_compile_options(truthraw_v47i_frozen_core_smldp PRIVATE -Wall -Wextra -Werror)
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(truthraw_v47i_frozen_core_smldp PRIVATE -Wno-error=misleading-indentation)
  endif()
endif()

add_executable(test_scientific_master_linear_dng_projection_v0_1
    tests/test_scientific_master_linear_dng_projection_v0_1.cpp
    native/scientific_master_linear_dng_projection_v0_1.cpp
    "${DIGEST}/scientific_master_digest_v0_1.cpp"
)

target_include_directories(test_scientific_master_linear_dng_projection_v0_1 PRIVATE
    native
    "${DIGEST}"
)

add_executable(test_streaming_scientific_master_tile_source_v0_1
    tests/test_streaming_scientific_master_tile_source_v0_1.cpp
    native/streaming_scientific_master_tile_source_v0_1.cpp
    "${SMSB2}/native/scientific_master_streaming_binding_v0_2.cpp"
    "${SMSB1}/native/scientific_master_streaming_binding_v0_1.cpp"
    "${DIGEST}/scientific_master_digest_v0_1.cpp"
    "${TRUTHRANGE}/native/truthrange_latent_v0_2.cpp"
    $<TARGET_OBJECTS:truthraw_v47i_frozen_core_smldp>
)

target_include_directories(test_streaming_scientific_master_tile_source_v0_1 PRIVATE
    native
    "${SMSB2}/native"
    "${SMSB1}/native"
    "${DIGEST}"
    "${TRUTHRANGE}/native"
    "${TILE_DNG}"
    "${STREAMING}/native"
    "${STREAMING}/tests"
    "${V47I}/include"
)

if(MSVC)
  target_compile_options(test_scientific_master_linear_dng_projection_v0_1 PRIVATE /W4 /WX)
  target_compile_options(test_streaming_scientific_master_tile_source_v0_1 PRIVATE /W4 /WX)
else()
  target_compile_options(test_scientific_master_linear_dng_projection_v0_1 PRIVATE
      -Wall -Wextra -Wpedantic -Werror)
  target_compile_options(test_streaming_scientific_master_tile_source_v0_1 PRIVATE
      -Wall -Wextra -Wpedantic -Werror)
endif()

if(TRUTHRAW_SMLDP_V01_ENABLE_SANITIZERS AND NOT MSVC)
  foreach(target
      test_scientific_master_linear_dng_projection_v0_1
      test_streaming_scientific_master_tile_source_v0_1)
    target_compile_options(${target} PRIVATE
        -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(${target} PRIVATE
        -fsanitize=address,undefined -fno-omit-frame-pointer)
  endforeach()
  target_compile_options(truthraw_v47i_frozen_core_smldp PRIVATE
      -fsanitize=address,undefined -fno-omit-frame-pointer)
endif()

enable_testing()
add_test(NAME scientific_master_linear_dng_projection_v0_1
         COMMAND test_scientific_master_linear_dng_projection_v0_1)
add_test(NAME streaming_scientific_master_tile_source_v0_1
         COMMAND test_streaming_scientific_master_tile_source_v0_1)
