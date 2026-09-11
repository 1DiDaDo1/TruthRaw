cmake_minimum_required(VERSION 3.22.1)
project(truthraw_ui_preview_bridge LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

get_filename_component(REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../../../../.." ABSOLUTE)
set(RESEARCH_ROOT "${REPO_ROOT}/docs/research")
set(TILE_DNG "${RESEARCH_ROOT}/tile-native-dng-source-v0.1/native")
set(STREAMING "${RESEARCH_ROOT}/full-frame-streaming-v0.1/native")
set(PREVIEW_SURFACE "${RESEARCH_ROOT}/reconstructed-color-preview-v0.1/native")
set(SOURCE_BINDING_V01 "${RESEARCH_ROOT}/scientific-preview-source-binding-v0.1/native")
set(SOURCE_BINDING_V02 "${RESEARCH_ROOT}/scientific-preview-source-binding-v0.2/native")
set(DNG_COLOR_V01 "${RESEARCH_ROOT}/dng-color-binding-producer-v0.1/native")
set(DNG_COLOR_V02 "${RESEARCH_ROOT}/dng-color-binding-producer-v0.2/native")
set(BACKPLANE "${RESEARCH_ROOT}/technical-backplane-v0.1/native")
set(BACKPLANE_PHASE2 "${RESEARCH_ROOT}/technical-backplane-phase2-v0.1/native")
set(SCIENTIFIC_MASTER_BINDING "${RESEARCH_ROOT}/scientific-master-streaming-binding-v0.1/native")
set(SCIENTIFIC_MASTER_DIGEST "${RESEARCH_ROOT}/scientific-master-digest-v0.1/native")
set(TRUTHRANGE_LATENT "${RESEARCH_ROOT}/truthrange-latent-binding-v0.2/native")
set(FINALIZED_RELEASE "${RESEARCH_ROOT}/finalized-scientific-preview-release-v0.1/native")
set(V47I_NATIVE "${REPO_ROOT}/canonical/reconstruction/v4.7i/native")

add_library(truthraw_ui_preview_bridge SHARED
    native_tile_preview_bridge.cpp
    source_bound_color_preview_bridge.cpp
    ${TILE_DNG}/tile_native_dng_source_v0_1.cpp
    ${TILE_DNG}/tile_native_dng_source_v0_1_bind.cpp
    ${TILE_DNG}/tile_native_dng_source_v0_1_read.cpp
    ${STREAMING}/full_frame_streaming_v0_1.cpp
    ${STREAMING}/full_frame_streaming_v0_1_pass1.cpp
    ${STREAMING}/full_frame_streaming_v0_1_pass2.cpp
    ${STREAMING}/full_frame_streaming_v0_1_plan.cpp
    ${PREVIEW_SURFACE}/bounded_srgb_preview_sink_v0_1.cpp
    ${SOURCE_BINDING_V01}/scientific_preview_source_binding_v0_1.cpp
    ${SOURCE_BINDING_V02}/scientific_preview_source_binding_v0_2.cpp
    ${DNG_COLOR_V01}/dng_color_binding_producer_v0_1.cpp
    ${DNG_COLOR_V02}/dng_color_binding_producer_v0_2.cpp
    ${BACKPLANE}/technical_backplane_v0_1.cpp
    ${BACKPLANE_PHASE2}/technical_backplane_phase2_v0_1.cpp
    ${SCIENTIFIC_MASTER_BINDING}/scientific_master_streaming_binding_v0_1.cpp
    ${SCIENTIFIC_MASTER_DIGEST}/scientific_master_digest_v0_1.cpp
    ${TRUTHRANGE_LATENT}/truthrange_latent_v0_2.cpp
    ${FINALIZED_RELEASE}/finalized_scientific_preview_release_v0_1.cpp
    ${V47I_NATIVE}/src/core.cpp)

target_include_directories(truthraw_ui_preview_bridge PRIVATE
    ${TILE_DNG}
    ${STREAMING}
    ${PREVIEW_SURFACE}
    ${SOURCE_BINDING_V01}
    ${SOURCE_BINDING_V02}
    ${DNG_COLOR_V01}
    ${DNG_COLOR_V02}
    ${BACKPLANE}
    ${BACKPLANE_PHASE2}
    ${SCIENTIFIC_MASTER_BINDING}
    ${SCIENTIFIC_MASTER_DIGEST}
    ${TRUTHRANGE_LATENT}
    ${FINALIZED_RELEASE}
    ${V47I_NATIVE}/include)

target_compile_options(truthraw_ui_preview_bridge PRIVATE -Wall -Wextra -Werror)

# canonical v4.7i is byte-frozen. Android NDK Clang 18 diagnoses one compact
# canonical loop/return line as -Wmisleading-indentation. Keep the diagnostic
# visible but non-fatal for this one frozen translation unit only; integration
# and research sources remain under the target-wide -Werror gate above.
set_source_files_properties(
    ${V47I_NATIVE}/src/core.cpp
    PROPERTIES COMPILE_OPTIONS "-Wno-error=misleading-indentation")
