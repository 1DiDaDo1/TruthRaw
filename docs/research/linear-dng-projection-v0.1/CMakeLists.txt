cmake_minimum_required(VERSION 3.16)
project(truthraw_linear_dng_projection_v0_1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(TRUTHRAW_LDNG01_ENABLE_SANITIZERS "Enable ASan/UBSan" OFF)

if(NOT DEFINED TRUTHRAW_REPO_ROOT)
  get_filename_component(TRUTHRAW_REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
endif()

set(V47I "${TRUTHRAW_REPO_ROOT}/canonical/reconstruction/v4.7i/native")
set(STREAMING "${TRUTHRAW_REPO_ROOT}/docs/research/full-frame-streaming-v0.1/native")
set(PREVIEW "${TRUTHRAW_REPO_ROOT}/docs/research/reconstructed-color-preview-v0.1/native")

add_executable(test_linear_dng_projection_v0_1
    tests/test_linear_dng_projection_v0_1_qualified.cpp
    native/linear_dng_projection_v0_1.cpp
)

target_include_directories(test_linear_dng_projection_v0_1 PRIVATE
    native
    "${V47I}/include"
    "${STREAMING}"
    "${PREVIEW}"
    ../finalized-scientific-preview-release-v0.2/native
    ../finalized-scientific-preview-release-v0.1/native
    ../scientific-master-streaming-binding-v0.2/native
    ../scientific-master-streaming-binding-v0.1/native
    ../scientific-master-digest-v0.1/native
    ../truthrange-latent-binding-v0.2/native
    ../technical-backplane-phase2-v0.1/native
    ../technical-backplane-v0.1/native
    ../scientific-preview-source-binding-v0.2/native
    ../scientific-preview-source-binding-v0.1/native
    ../tile-native-dng-source-v0.1/native
)

if(MSVC)
  target_compile_options(test_linear_dng_projection_v0_1 PRIVATE /W4 /WX)
else()
  target_compile_options(test_linear_dng_projection_v0_1 PRIVATE
      -Wall -Wextra -Wpedantic -Werror)
endif()

if(TRUTHRAW_LDNG01_ENABLE_SANITIZERS AND NOT MSVC)
  target_compile_options(test_linear_dng_projection_v0_1 PRIVATE
      -fsanitize=address,undefined -fno-omit-frame-pointer)
  target_link_options(test_linear_dng_projection_v0_1 PRIVATE
      -fsanitize=address,undefined -fno-omit-frame-pointer)
endif()

enable_testing()
add_test(NAME linear_dng_projection_v0_1
         COMMAND test_linear_dng_projection_v0_1)
