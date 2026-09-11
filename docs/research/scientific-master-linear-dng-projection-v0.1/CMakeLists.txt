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

add_executable(test_scientific_master_linear_dng_projection_v0_1
    tests/test_scientific_master_linear_dng_projection_v0_1.cpp
    native/scientific_master_linear_dng_projection_v0_1.cpp
    "${DIGEST}/scientific_master_digest_v0_1.cpp"
)

target_include_directories(test_scientific_master_linear_dng_projection_v0_1 PRIVATE
    native
    "${DIGEST}"
)

if(MSVC)
  target_compile_options(test_scientific_master_linear_dng_projection_v0_1 PRIVATE /W4 /WX)
else()
  target_compile_options(test_scientific_master_linear_dng_projection_v0_1 PRIVATE
      -Wall -Wextra -Wpedantic -Werror)
endif()

if(TRUTHRAW_SMLDP_V01_ENABLE_SANITIZERS AND NOT MSVC)
  target_compile_options(test_scientific_master_linear_dng_projection_v0_1 PRIVATE
      -fsanitize=address,undefined -fno-omit-frame-pointer)
  target_link_options(test_scientific_master_linear_dng_projection_v0_1 PRIVATE
      -fsanitize=address,undefined -fno-omit-frame-pointer)
endif()

enable_testing()
add_test(NAME scientific_master_linear_dng_projection_v0_1
         COMMAND test_scientific_master_linear_dng_projection_v0_1)
