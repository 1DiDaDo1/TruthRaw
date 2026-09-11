cmake_minimum_required(VERSION 3.16)
project(truthraw_finalized_scientific_preview_release_v0_2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(TRUTHRAW_FSPR_V02_ENABLE_SANITIZERS "Enable ASan/UBSan" OFF)

if(NOT DEFINED TRUTHRAW_REPO_ROOT)
  get_filename_component(TRUTHRAW_REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
endif()

set(V47I "${TRUTHRAW_REPO_ROOT}/canonical/reconstruction/v4.7i/native")
set(STREAMING "${TRUTHRAW_REPO_ROOT}/docs/research/full-frame-streaming-v0.1/native")
set(PREVIEW "${TRUTHRAW_REPO_ROOT}/docs/research/reconstructed-color-preview-v0.1/native")

add_library(truthraw_v47i_frozen_core_fspr_v02 OBJECT "${V47I}/src/core.cpp")
target_include_directories(truthraw_v47i_frozen_core_fspr_v02 PRIVATE "${V47I}/include")
target_compile_options(truthraw_v47i_frozen_core_fspr_v02 PRIVATE -Wall -Wextra -Werror)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  target_compile_options(truthraw_v47i_frozen_core_fspr_v02 PRIVATE -Wno-error=misleading-indentation)
endif()

set(FSPR_V02_COMMON_SOURCES
    native/finalized_scientific_preview_release_v0_2.cpp
    ../finalized-scientific-preview-release-v0.1/native/finalized_scientific_preview_release_v0_1.cpp
    "${PREVIEW}/bounded_srgb_preview_sink_v0_1.cpp"
    "${STREAMING}/full_frame_streaming_v0_1.cpp"
    "${STREAMING}/full_frame_streaming_v0_1_plan.cpp"
    "${STREAMING}/full_frame_streaming_v0_1_pass1.cpp"
    "${STREAMING}/full_frame_streaming_v0_1_pass2.cpp"
    ../scientific-master-streaming-binding-v0.1/native/scientific_master_streaming_binding_v0_1.cpp
    ../scientific-master-streaming-binding-v0.2/native/scientific_master_streaming_binding_v0_2.cpp
    ../scientific-master-digest-v0.1/native/scientific_master_digest_v0_1.cpp
    ../truthrange-latent-binding-v0.2/native/truthrange_latent_v0_2.cpp
    ../technical-backplane-phase2-v0.1/native/technical_backplane_phase2_v0_1.cpp
    ../technical-backplane-v0.1/native/technical_backplane_v0_1.cpp
    ../scientific-preview-source-binding-v0.1/native/scientific_preview_source_binding_v0_1.cpp
    ../scientific-preview-source-binding-v0.2/native/scientific_preview_source_binding_v0_2.cpp
    $<TARGET_OBJECTS:truthraw_v47i_frozen_core_fspr_v02>
)

set(FSPR_V02_COMMON_INCLUDES
    native
    tests
    ../finalized-scientific-preview-release-v0.1/native
    ../finalized-scientific-preview-release-v0.1/tests
    "${PREVIEW}"
    "${STREAMING}"
    ../scientific-master-streaming-binding-v0.1/native
    ../scientific-master-streaming-binding-v0.2/native
    ../scientific-master-digest-v0.1/native
    ../truthrange-latent-binding-v0.2/native
    ../technical-backplane-phase2-v0.1/native
    ../technical-backplane-v0.1/native
    ../scientific-preview-source-binding-v0.1/native
    ../scientific-preview-source-binding-v0.2/native
    ../tile-native-dng-source-v0.1/native
    "${V47I}/include"
)

add_executable(test_finalized_scientific_preview_release_v0_2
    tests/test_finalized_scientific_preview_release_v0_2.cpp
    ${FSPR_V02_COMMON_SOURCES}
)
target_include_directories(test_finalized_scientific_preview_release_v0_2 PRIVATE
    ${FSPR_V02_COMMON_INCLUDES})

if(MSVC)
  target_compile_options(test_finalized_scientific_preview_release_v0_2 PRIVATE /W4 /WX)
else()
  target_compile_options(test_finalized_scientific_preview_release_v0_2 PRIVATE
      -Wall -Wextra -Wpedantic -Werror)
endif()

if(TRUTHRAW_FSPR_V02_ENABLE_SANITIZERS AND NOT MSVC)
  target_compile_options(test_finalized_scientific_preview_release_v0_2 PRIVATE
      -fsanitize=address,undefined -fno-omit-frame-pointer)
  target_link_options(test_finalized_scientific_preview_release_v0_2 PRIVATE
      -fsanitize=address,undefined -fno-omit-frame-pointer)
endif()

enable_testing()
add_test(NAME finalized_scientific_preview_release_v0_2
         COMMAND test_finalized_scientific_preview_release_v0_2)
