cmake_minimum_required(VERSION 3.20)
project(draw_host_scientific_route_v0_1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

get_filename_component(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
set(APP "${REPO_ROOT}/suite_android/app/src/main/cpp")
set(RESEARCH "${REPO_ROOT}/docs/research")
set(TILE "${RESEARCH}/tile-native-dng-source-v0.1/native")
set(STREAM "${RESEARCH}/full-frame-streaming-v0.1/native")
set(SOURCE1 "${RESEARCH}/scientific-preview-source-binding-v0.1/native")
set(SOURCE2 "${RESEARCH}/scientific-preview-source-binding-v0.2/native")
set(DNG1 "${RESEARCH}/dng-color-binding-producer-v0.1/native")
set(DNG2 "${RESEARCH}/dng-color-binding-producer-v0.2/native")
set(BACK "${RESEARCH}/technical-backplane-v0.1/native")
set(BACK2 "${RESEARCH}/technical-backplane-phase2-v0.1/native")
set(SMSB1 "${RESEARCH}/scientific-master-streaming-binding-v0.1/native")
set(SMSB2 "${RESEARCH}/scientific-master-streaming-binding-v0.2/native")
set(DIGEST "${RESEARCH}/scientific-master-digest-v0.1/native")
set(TRUTHRANGE "${RESEARCH}/truthrange-latent-binding-v0.2/native")
set(MULTI "${RESEARCH}/multivendor-raw-source-adapter-v0.1/native")
set(F64 "${RESEARCH}/scientific-master-f64-reconstruction-v0.1/native")
set(SMLDP "${RESEARCH}/scientific-master-linear-dng-projection-v0.1/native")
set(TNCONT "${RESEARCH}/truthnegative-continuous-v0.5/native")
set(DRAWNEG "${RESEARCH}/drawnegative-v0.1/native")
set(FREEWORLD "${RESEARCH}/free-world-pixel-resolve-2d-v0.2/native")
set(DENSE "${RESEARCH}/truthnegative-dense-projection-v0.3/native")
set(V47I "${REPO_ROOT}/canonical/reconstruction/v4.7i/native")

find_package(Threads REQUIRED)

add_executable(draw_host_scientific_route_v0_1
    host/draw_host_scientific_route_v0_1.cpp
    "${APP}/truthnegative_pipeline_bridge_common.cpp"
    "${APP}/truthnegative_dense_local_field_adapter_v0_4.cpp"
    "${APP}/truthnegative_local_authority_projection_v0_4.cpp"
    "${APP}/open_scene_field_v0_85.cpp"
    "${APP}/open_scene_channel_authority_v0_78.cpp"
    "${APP}/truthraw_sha256_v0_69.cpp"
    "${TILE}/tile_native_dng_source_v0_1.cpp"
    "${TILE}/tile_native_dng_source_v0_1_bind.cpp"
    "${TILE}/tile_native_dng_source_v0_1_read.cpp"
    "${MULTI}/multivendor_raw_source_adapter_v0_1.cpp"
    "${SOURCE1}/scientific_preview_source_binding_v0_1.cpp"
    "${SOURCE2}/scientific_preview_source_binding_v0_2.cpp"
    "${DNG1}/dng_color_binding_producer_v0_1.cpp"
    "${DNG2}/dng_color_binding_producer_v0_2.cpp"
    "${BACK}/technical_backplane_v0_1.cpp"
    "${BACK2}/technical_backplane_phase2_v0_1.cpp"
    "${SMSB1}/scientific_master_streaming_binding_v0_1.cpp"
    "${SMSB2}/scientific_master_streaming_binding_v0_2.cpp"
    "${DIGEST}/scientific_master_digest_v0_1.cpp"
    "${TRUTHRANGE}/truthrange_latent_v0_2.cpp"
    "${F64}/scientific_master_f64_reconstruction_v0_1.cpp"
    "${V47I}/src/core.cpp"
    "${SMLDP}/scientific_master_linear_dng_projection_v0_1.cpp"
    "${SMLDP}/streaming_scientific_master_tile_source_v0_1.cpp"
    "${TNCONT}/truthnegative_continuous_v0_5.cpp"
    "${DRAWNEG}/drawnegative_v0_1.cpp"
    "${FREEWORLD}/free_world_pixel_resolve_2d_v0_2.cpp"
    "${DENSE}/truthnegative_dense_projection_v0_3.cpp"
)

target_include_directories(draw_host_scientific_route_v0_1 PRIVATE
    "${APP}"
    "${TILE}"
    "${STREAM}"
    "${SOURCE1}"
    "${SOURCE2}"
    "${DNG1}"
    "${DNG2}"
    "${BACK}"
    "${BACK2}"
    "${SMSB1}"
    "${SMSB2}"
    "${DIGEST}"
    "${TRUTHRANGE}"
    "${MULTI}"
    "${F64}"
    "${SMLDP}"
    "${TNCONT}"
    "${DRAWNEG}"
    "${FREEWORLD}"
    "${DENSE}"
    "${V47I}/include"
)

target_link_libraries(draw_host_scientific_route_v0_1 PRIVATE Threads::Threads)

if(MSVC)
    target_compile_options(draw_host_scientific_route_v0_1 PRIVATE /W4 /WX)
else()
    target_compile_options(draw_host_scientific_route_v0_1 PRIVATE
        -O2 -Wall -Wextra -Wpedantic -Werror
        -fno-fast-math -ffp-contract=off)
    set_source_files_properties(
        "${V47I}/src/core.cpp"
        PROPERTIES COMPILE_OPTIONS "-Wno-error=misleading-indentation")
endif()
