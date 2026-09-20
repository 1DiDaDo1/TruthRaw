cmake_minimum_required(VERSION 3.22.1)
project(truthraw_ui_preview_bridge LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Recovered 2026-09-14 app now lives at <repo>/suite_android instead of
# <repo>/app/android/truthraw-adaptive-ui-v01. Keep every scientific source
# path repository-relative; only this integration path changes.
get_filename_component(REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../../.." ABSOLUTE)
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
set(SCIENTIFIC_MASTER_BINDING_V01 "${RESEARCH_ROOT}/scientific-master-streaming-binding-v0.1/native")
set(SCIENTIFIC_MASTER_BINDING_V02 "${RESEARCH_ROOT}/scientific-master-streaming-binding-v0.2/native")
set(SCIENTIFIC_MASTER_BINDING_V03 "${RESEARCH_ROOT}/scientific-master-streaming-binding-v0.3/native")
set(SCIENTIFIC_MASTER_DIGEST "${RESEARCH_ROOT}/scientific-master-digest-v0.1/native")
set(SCIENTIFIC_MASTER_FLOAT_DNG "${RESEARCH_ROOT}/scientific-master-linear-dng-projection-v0.1/native")
set(TRUTHRANGE_LATENT "${RESEARCH_ROOT}/truthrange-latent-binding-v0.2/native")
set(FINALIZED_RELEASE_V01 "${RESEARCH_ROOT}/finalized-scientific-preview-release-v0.1/native")
set(FINALIZED_RELEASE_V02 "${RESEARCH_ROOT}/finalized-scientific-preview-release-v0.2/native")
set(FINALIZED_RELEASE_V03 "${RESEARCH_ROOT}/finalized-scientific-preview-release-v0.3/native")
set(LINEAR_DNG_PROJECTION "${RESEARCH_ROOT}/linear-dng-projection-v0.1/native")
set(MULTIVENDOR_RAW_ADAPTER "${RESEARCH_ROOT}/multivendor-raw-source-adapter-v0.1/native")
set(OPEN_WORLD_NATIVE_V03 "${RESEARCH_ROOT}/open-world-native-integration-v0.3/native")
set(CICM_V1 "${RESEARCH_ROOT}/counterfactual-illumination-capture-manifold-v1/native")
set(ROOM_CAPSULE_V01 "${RESEARCH_ROOT}/local-illumination-room-capsule-v0.1/native")
set(V47I_NATIVE "${REPO_ROOT}/canonical/reconstruction/v4.7i/native")

add_library(truthraw_ui_preview_bridge SHARED
    native_tile_preview_bridge.cpp
    source_bound_color_preview_bridge.cpp
    advanced_preview_bridge.cpp
    empirical_source_probe.cpp
    linear_dng_export_bridge.cpp
    pure_float32_dng_export_bridge.cpp
    truthnegative_export_bridge.cpp
    fullres_restoration_export_bridge.cpp
    restoration_projection_bridge.cpp
    truthraw_sha256_v0_69.cpp
    canonical_ancestry_v0_77.cpp
    open_scene_canonical_v0_70.cpp
    open_scene_channel_authority_v0_78.cpp
    bound_uncertainty_admission_v0_79.cpp
    adaptive_detail_v47j_adapter.cpp
    nef_measurement_preview_bridge.cpp
    hardware_buffer_envelope_probe.cpp
    idealraw_native_type_oracle.cpp
    rawcb_source_type_native_type_oracle.cpp
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
    ${SCIENTIFIC_MASTER_BINDING_V01}/scientific_master_streaming_binding_v0_1.cpp
    ${SCIENTIFIC_MASTER_BINDING_V02}/scientific_master_streaming_binding_v0_2.cpp
    ${SCIENTIFIC_MASTER_BINDING_V03}/scientific_master_streaming_binding_v0_3.cpp
    ${SCIENTIFIC_MASTER_DIGEST}/scientific_master_digest_v0_1.cpp
    ${SCIENTIFIC_MASTER_FLOAT_DNG}/scientific_master_linear_dng_projection_v0_1.cpp
    ${SCIENTIFIC_MASTER_FLOAT_DNG}/streaming_scientific_master_tile_source_v0_1.cpp
    ${TRUTHRANGE_LATENT}/truthrange_latent_v0_2.cpp
    ${FINALIZED_RELEASE_V01}/finalized_scientific_preview_release_v0_1.cpp
    ${FINALIZED_RELEASE_V02}/finalized_scientific_preview_release_v0_2.cpp
    ${FINALIZED_RELEASE_V03}/finalized_scientific_preview_release_v0_3.cpp
    ${LINEAR_DNG_PROJECTION}/linear_dng_projection_v0_1.cpp
    ${MULTIVENDOR_RAW_ADAPTER}/multivendor_raw_source_adapter_v0_1.cpp
    ${MULTIVENDOR_RAW_ADAPTER}/nikon_nef_uncompressed_adapter_v0_1.cpp
    ${OPEN_WORLD_NATIVE_V03}/open_world_native_v03.cpp
    ${CICM_V1}/cicm_v1.cpp
    ${ROOM_CAPSULE_V01}/room_capsule_v0_1.cpp
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
    ${SCIENTIFIC_MASTER_BINDING_V01}
    ${SCIENTIFIC_MASTER_BINDING_V02}
    ${SCIENTIFIC_MASTER_BINDING_V03}
    ${SCIENTIFIC_MASTER_DIGEST}
    ${SCIENTIFIC_MASTER_FLOAT_DNG}
    ${TRUTHRANGE_LATENT}
    ${FINALIZED_RELEASE_V01}
    ${FINALIZED_RELEASE_V02}
    ${FINALIZED_RELEASE_V03}
    ${LINEAR_DNG_PROJECTION}
    ${MULTIVENDOR_RAW_ADAPTER}
    ${OPEN_WORLD_NATIVE_V03}
    ${CICM_V1}
    ${ROOM_CAPSULE_V01}
    ${V47I_NATIVE}/include)

find_library(ANDROID_LIB android)
find_library(CAMERA2NDK_LIB camera2ndk)
find_library(DL_LIB dl)
target_link_libraries(truthraw_ui_preview_bridge PRIVATE ${ANDROID_LIB} ${CAMERA2NDK_LIB} ${DL_LIB})

target_compile_options(truthraw_ui_preview_bridge PRIVATE -Wall -Wextra -Werror)

# canonical v4.7i is byte-frozen. Android NDK Clang 18 diagnoses one compact
# canonical loop/return line as -Wmisleading-indentation. Keep the diagnostic
# visible but non-fatal for this one frozen translation unit only; integration
# and research sources remain under the target-wide -Werror gate above.
set_source_files_properties(
    ${V47I_NATIVE}/src/core.cpp
    PROPERTIES COMPILE_OPTIONS "-Wno-error=misleading-indentation")
