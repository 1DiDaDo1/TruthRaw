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
set(TRUTHNEGATIVE_DENSE_V03 "${RESEARCH_ROOT}/truthnegative-dense-projection-v0.3/native")
set(TRUTHRANGE_LATENT "${RESEARCH_ROOT}/truthrange-latent-binding-v0.2/native")
set(TRUTHRANGE_DENSE_UNCERTAINTY_V03 "${RESEARCH_ROOT}/truthrange-dense-uncertainty-v0.3/native")
set(FINALIZED_RELEASE_V01 "${RESEARCH_ROOT}/finalized-scientific-preview-release-v0.1/native")
set(FINALIZED_RELEASE_V02 "${RESEARCH_ROOT}/finalized-scientific-preview-release-v0.2/native")
set(FINALIZED_RELEASE_V03 "${RESEARCH_ROOT}/finalized-scientific-preview-release-v0.3/native")
set(LINEAR_DNG_PROJECTION "${RESEARCH_ROOT}/linear-dng-projection-v0.1/native")
set(MULTIVENDOR_RAW_ADAPTER "${RESEARCH_ROOT}/multivendor-raw-source-adapter-v0.1/native")
set(OPEN_WORLD_NATIVE_V03 "${RESEARCH_ROOT}/open-world-native-integration-v0.3/native")
set(CICM_V1 "${RESEARCH_ROOT}/counterfactual-illumination-capture-manifold-v1/native")
set(ROOM_CAPSULE_V01 "${RESEARCH_ROOT}/local-illumination-room-capsule-v0.1/native")
set(V47I_NATIVE "${REPO_ROOT}/canonical/reconstruction/v4.7i/native")
set(F64_RECON_NATIVE "${RESEARCH_ROOT}/scientific-master-f64-reconstruction-v0.1/native")
set(V47K_NATIVE "${REPO_ROOT}/canonical/output-acutance/v4.7k/native")
set(FREE_WORLD_V02 "${RESEARCH_ROOT}/free-world-pixel-resolve-2d-v0.2/native")
set(FREE_WORLD_V03 "${RESEARCH_ROOT}/free-world-scientific-open-scene-binding-v0.3/native")
set(FREE_WORLD_V04 "${RESEARCH_ROOT}/free-world-deep-scene-contribution-v0.4/native")
set(FREE_WORLD_V05 "${RESEARCH_ROOT}/free-world-deep-scene-binding-v0.5/native")
set(FREE_WORLD_V06 "${RESEARCH_ROOT}/free-world-light-transport-state-v0.6/native")
set(FREE_WORLD_V07 "${RESEARCH_ROOT}/free-world-appearance-resolve-v0.7/native")
set(FREE_WORLD_NOISE_FUSION_V01 "${RESEARCH_ROOT}/free-world-observation-noise-fusion-v0.1/native")
set(TRUTHNEGATIVE_CONTINUOUS_V05 "${RESEARCH_ROOT}/truthnegative-continuous-v0.5/native")
set(TRUTHNEGATIVE_NOISE_V01 "${RESEARCH_ROOT}/truthnegative-noise-state-v0.1/native")
set(TRUTHNEGATIVE_STRUCTURE_GATE_V01 "${RESEARCH_ROOT}/truthnegative-structure-preservation-gate-v0.1/native")
set(TRUTHNEGATIVE_RESIDUAL_V01 "${RESEARCH_ROOT}/truthnegative-bounded-residual-estimator-v0.1/native")
set(TRUTHNEGATIVE_NEIGHBORHOOD_V01 "${RESEARCH_ROOT}/truthnegative-authority-aware-neighborhood-v0.1/native")
set(TRUTHNEGATIVE_N2_PIPELINE_V01 "${RESEARCH_ROOT}/truthnegative-n2-candidate-pipeline-v0.1/native")
set(TRUTHNEGATIVE_N2_CFA_AUDIT_V01 "${RESEARCH_ROOT}/truthnegative-n2-cfa-audit-v0.1/native")
set(TRUTHNEGATIVE_N2_SPATIAL_V01 "${RESEARCH_ROOT}/truthnegative-n2-spatial-sidecar-v0.1/native")
set(TRUTHNEGATIVE_N2_FULL_COLOUR_V01 "${RESEARCH_ROOT}/truthnegative-n2-full-colour-candidate-v0.1/native")
set(TRUTHNEGATIVE_ROUNDTRIP_V06 "${RESEARCH_ROOT}/truthnegative-roundtrip-oracle-v0.6/native")
set(TRUTHNEGATIVE_OPTICS_V07 "${RESEARCH_ROOT}/truthnegative-optics-support-v0.7/native")
set(TRUTHNEGATIVE_DEEP_V08 "${RESEARCH_ROOT}/truthnegative-deep-scene-bridge-v0.8/native")
set(TRUTHNEGATIVE_CAMERA5_ORACLE_V01 "${RESEARCH_ROOT}/truthnegative-camera5-color-highlight-oracle-v0.1/native")
set(TRUTHNEGATIVE_NATIVE_CONTAINER_V01 "${RESEARCH_ROOT}/truthnegative-native-container-v0.1/native")

add_library(truthraw_ui_preview_bridge SHARED
    native_tile_preview_bridge.cpp
    source_bound_color_preview_bridge.cpp
    advanced_preview_bridge.cpp
    truthnegative_continuous_preview_bridge.cpp
    truthnegative_pipeline_bridge_common.cpp
    camera5_color_highlight_oracle_bridge.cpp
    truthnegative_native_container_bridge.cpp
    truthnegative_n2_spatial_sidecar_bridge.cpp
    truthnegative_n2_crop_ab_bridge.cpp
    photo_export_bridge.cpp
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
    open_scene_field_v0_85.cpp
    open_scene_local_policy_v0_86.cpp
    truthnegative_local_authority_projection_v0_4.cpp
    truthnegative_dense_local_field_adapter_v0_4.cpp
    output_channel_authority_v0_84.cpp
    truthraw_compute_capabilities_v0_1.cpp
    truthraw_system_headroom_v0_1.cpp
    truthraw_performance_hint_v0_1.cpp
    truthnegative_vulkan_dense_v0_1.cpp
    unified_output_preview_v0_1.cpp
    unified_output_preview_sources_v0_1.cpp
    bound_uncertainty_admission_v0_79.cpp
    adaptive_detail_v47j_adapter.cpp
    advanced_render_edit_tile_source_v0_1.cpp
    output_acutance_v0_81.cpp
    illumination_state_v0_82.cpp
    hdr_authority_v0_83.cpp
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
    ${TRUTHNEGATIVE_DENSE_V03}/truthnegative_dense_projection_v0_3.cpp
    ${TRUTHRANGE_LATENT}/truthrange_latent_v0_2.cpp
    ${TRUTHRANGE_DENSE_UNCERTAINTY_V03}/truthrange_dense_uncertainty_v0_3.cpp
    ${FINALIZED_RELEASE_V01}/finalized_scientific_preview_release_v0_1.cpp
    ${FINALIZED_RELEASE_V02}/finalized_scientific_preview_release_v0_2.cpp
    ${FINALIZED_RELEASE_V03}/finalized_scientific_preview_release_v0_3.cpp
    ${LINEAR_DNG_PROJECTION}/linear_dng_projection_v0_1.cpp
    ${MULTIVENDOR_RAW_ADAPTER}/multivendor_raw_source_adapter_v0_1.cpp
    ${MULTIVENDOR_RAW_ADAPTER}/nikon_nef_uncompressed_adapter_v0_1.cpp
    ${OPEN_WORLD_NATIVE_V03}/open_world_native_v03.cpp
    ${CICM_V1}/cicm_v1.cpp
    ${ROOM_CAPSULE_V01}/room_capsule_v0_1.cpp
    ${V47I_NATIVE}/src/core.cpp
    ${F64_RECON_NATIVE}/scientific_master_f64_reconstruction_v0_1.cpp
    ${V47K_NATIVE}/output_acutance.cpp
    ${FREE_WORLD_V02}/free_world_pixel_resolve_2d_v0_2.cpp
    ${FREE_WORLD_V03}/free_world_scientific_open_scene_binding_v0_3.cpp
    ${FREE_WORLD_V04}/free_world_deep_scene_contribution_v0_4.cpp
    ${FREE_WORLD_V05}/free_world_deep_scene_binding_v0_5.cpp
    ${FREE_WORLD_V06}/free_world_light_transport_state_v0_6.cpp
    ${FREE_WORLD_V07}/free_world_appearance_resolve_v0_7.cpp
    ${FREE_WORLD_NOISE_FUSION_V01}/free_world_observation_noise_fusion_v0_1.cpp
    ${TRUTHNEGATIVE_CONTINUOUS_V05}/truthnegative_continuous_v0_5.cpp
    ${TRUTHNEGATIVE_NOISE_V01}/truthnegative_noise_state_v0_1.cpp
    ${TRUTHNEGATIVE_NOISE_V01}/truthnegative_dense_uncertainty_adapter_v0_1.cpp
    ${TRUTHNEGATIVE_STRUCTURE_GATE_V01}/truthnegative_structure_preservation_gate_v0_1.cpp
    ${TRUTHNEGATIVE_RESIDUAL_V01}/truthnegative_bounded_residual_estimator_v0_1.cpp
    ${TRUTHNEGATIVE_NEIGHBORHOOD_V01}/truthnegative_authority_aware_neighborhood_v0_1.cpp
    ${TRUTHNEGATIVE_N2_PIPELINE_V01}/truthnegative_n2_candidate_pipeline_v0_1.cpp
    ${TRUTHNEGATIVE_N2_CFA_AUDIT_V01}/truthnegative_n2_cfa_audit_v0_1.cpp
    ${TRUTHNEGATIVE_N2_SPATIAL_V01}/truthnegative_n2_spatial_sidecar_v0_1.cpp
    ${TRUTHNEGATIVE_N2_FULL_COLOUR_V01}/truthnegative_n2_full_colour_candidate_v0_1.cpp
    ${TRUTHNEGATIVE_ROUNDTRIP_V06}/truthnegative_roundtrip_oracle_v0_6.cpp
    ${TRUTHNEGATIVE_OPTICS_V07}/truthnegative_optics_support_v0_7.cpp
    ${TRUTHNEGATIVE_DEEP_V08}/truthnegative_deep_scene_bridge_v0_8.cpp
    ${TRUTHNEGATIVE_CAMERA5_ORACLE_V01}/truthnegative_camera5_color_highlight_oracle_v0_1.cpp
    ${TRUTHNEGATIVE_NATIVE_CONTAINER_V01}/truthnegative_native_container_v0_1.cpp)

target_include_directories(truthraw_ui_preview_bridge PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
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
    ${TRUTHNEGATIVE_DENSE_V03}
    ${TRUTHRANGE_LATENT}
    ${TRUTHRANGE_DENSE_UNCERTAINTY_V03}
    ${FINALIZED_RELEASE_V01}
    ${FINALIZED_RELEASE_V02}
    ${FINALIZED_RELEASE_V03}
    ${LINEAR_DNG_PROJECTION}
    ${MULTIVENDOR_RAW_ADAPTER}
    ${OPEN_WORLD_NATIVE_V03}
    ${CICM_V1}
    ${ROOM_CAPSULE_V01}
    ${V47I_NATIVE}/include
    ${F64_RECON_NATIVE}
    ${V47K_NATIVE}
    ${FREE_WORLD_V02}
    ${FREE_WORLD_V03}
    ${FREE_WORLD_V04}
    ${FREE_WORLD_V05}
    ${FREE_WORLD_V06}
    ${FREE_WORLD_V07}
    ${FREE_WORLD_NOISE_FUSION_V01}
    ${TRUTHNEGATIVE_CONTINUOUS_V05}
    ${TRUTHNEGATIVE_NOISE_V01}
    ${TRUTHNEGATIVE_STRUCTURE_GATE_V01}
    ${TRUTHNEGATIVE_RESIDUAL_V01}
    ${TRUTHNEGATIVE_NEIGHBORHOOD_V01}
    ${TRUTHNEGATIVE_N2_PIPELINE_V01}
    ${TRUTHNEGATIVE_N2_CFA_AUDIT_V01}
    ${TRUTHNEGATIVE_N2_SPATIAL_V01}
    ${TRUTHNEGATIVE_N2_FULL_COLOUR_V01}
    ${TRUTHNEGATIVE_ROUNDTRIP_V06}
    ${TRUTHNEGATIVE_OPTICS_V07}
    ${TRUTHNEGATIVE_DEEP_V08}
    ${TRUTHNEGATIVE_CAMERA5_ORACLE_V01}
    ${TRUTHNEGATIVE_NATIVE_CONTAINER_V01})

if(ANDROID)
    file(GLOB TRUTHRAW_GLSLC_CANDIDATES
        "${CMAKE_ANDROID_NDK}/shader-tools/*/glslc*")
    list(LENGTH TRUTHRAW_GLSLC_CANDIDATES TRUTHRAW_GLSLC_COUNT)
    if(TRUTHRAW_GLSLC_COUNT EQUAL 0)
        message(FATAL_ERROR "NDK glslc not found under ${CMAKE_ANDROID_NDK}/shader-tools")
    endif()
    list(GET TRUTHRAW_GLSLC_CANDIDATES 0 TRUTHRAW_GLSLC)

    set(TRUTHRAW_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
    set(TRUTHRAW_DENSE_SHADER
        "${CMAKE_CURRENT_LIST_DIR}/shaders/truthnegative_dense_4x.comp")
    set(TRUTHRAW_DENSE_SPV
        "${TRUTHRAW_GENERATED_DIR}/truthnegative_dense_4x.comp.spv")
    set(TRUTHRAW_DENSE_SPV_HEADER
        "${TRUTHRAW_GENERATED_DIR}/truthnegative_dense_4x_spv.h")
    add_custom_command(
        OUTPUT "${TRUTHRAW_DENSE_SPV_HEADER}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${TRUTHRAW_GENERATED_DIR}"
        COMMAND "${TRUTHRAW_GLSLC}"
            -fshader-stage=compute
            -O0
            --target-env=vulkan1.1
            "${TRUTHRAW_DENSE_SHADER}"
            -o "${TRUTHRAW_DENSE_SPV}"
        COMMAND "${CMAKE_COMMAND}"
            -DINPUT=${TRUTHRAW_DENSE_SPV}
            -DOUTPUT=${TRUTHRAW_DENSE_SPV_HEADER}
            -P "${CMAKE_CURRENT_LIST_DIR}/cmake/embed_spirv.cmake"
        DEPENDS
            "${TRUTHRAW_DENSE_SHADER}"
            "${CMAKE_CURRENT_LIST_DIR}/cmake/embed_spirv.cmake"
        VERBATIM)
    add_custom_target(
        truthraw_dense_spirv
        DEPENDS "${TRUTHRAW_DENSE_SPV_HEADER}")
    add_dependencies(truthraw_ui_preview_bridge truthraw_dense_spirv)
    target_include_directories(
        truthraw_ui_preview_bridge PRIVATE "${TRUTHRAW_GENERATED_DIR}")
endif()

find_library(ANDROID_LIB android)
find_library(CAMERA2NDK_LIB camera2ndk)
find_library(DL_LIB dl)
find_library(VULKAN_LIB vulkan)
target_link_libraries(
    truthraw_ui_preview_bridge PRIVATE
    ${ANDROID_LIB} ${CAMERA2NDK_LIB} ${DL_LIB} ${VULKAN_LIB})

target_compile_options(truthraw_ui_preview_bridge PRIVATE
    -Wall
    -Wextra
    -Werror
    # The Android app is distributed/tested as a debug APK. Without an
    # explicit optimization level, its native imaging core otherwise runs
    # essentially unoptimized. O2 is admitted only with strict FP semantics:
    # no fast-math and no implicit fused contraction. CI compares the exact
    # v4.7i Float32 output signature at O0 vs O2 before this APK is built.
    $<$<CONFIG:Debug>:-O2>
    $<$<CONFIG:Debug>:-fno-fast-math>
    $<$<CONFIG:Debug>:-ffp-contract=off>)

# canonical v4.7i is byte-frozen. Android NDK Clang 18 diagnoses one compact
# canonical loop/return line as -Wmisleading-indentation. Keep the diagnostic
# visible but non-fatal for this one frozen translation unit only; integration
# and research sources remain under the target-wide -Werror gate above.
set_source_files_properties(
    ${V47I_NATIVE}/src/core.cpp
    PROPERTIES COMPILE_OPTIONS "-Wno-error=misleading-indentation")
