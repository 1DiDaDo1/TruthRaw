cmake_minimum_required(VERSION 3.16)
project(truthnegative_authority_field_fusion_v0_1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

get_filename_component(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
set(APP_CPP "${REPO_ROOT}/suite_android/app/src/main/cpp")
set(V47I "${REPO_ROOT}/canonical/reconstruction/v4.7i/native")
set(STREAM "${REPO_ROOT}/docs/research/full-frame-streaming-v0.1/native")
set(SMSB1 "${REPO_ROOT}/docs/research/scientific-master-streaming-binding-v0.1/native")
set(SMSB2 "${REPO_ROOT}/docs/research/scientific-master-streaming-binding-v0.2/native")
set(DIGEST "${REPO_ROOT}/docs/research/scientific-master-digest-v0.1/native")
set(TRUTHRANGE "${REPO_ROOT}/docs/research/truthrange-latent-binding-v0.2/native")
set(MASTER_TILE "${REPO_ROOT}/docs/research/scientific-master-linear-dng-projection-v0.1/native")
set(TN "${REPO_ROOT}/docs/research/truthnegative-continuous-v0.5/native")
set(FREE_WORLD "${REPO_ROOT}/docs/research/free-world-pixel-resolve-2d-v0.2/native")

add_library(
    truthraw_v47i_frozen_core_authority_fusion
    OBJECT "${V47I}/src/core.cpp")
target_include_directories(
    truthraw_v47i_frozen_core_authority_fusion
    PRIVATE "${V47I}/include")
target_compile_options(
    truthraw_v47i_frozen_core_authority_fusion
    PRIVATE -Wall -Wextra -Werror)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(
        truthraw_v47i_frozen_core_authority_fusion
        PRIVATE -Wno-error=misleading-indentation)
endif()

add_executable(
    test_truthnegative_authority_field_fusion_v0_1
    tests/test_truthnegative_authority_field_fusion_v0_1.cpp
    "${SMSB2}/scientific_master_streaming_binding_v0_2.cpp"
    "${DIGEST}/scientific_master_digest_v0_1.cpp"
    "${TRUTHRANGE}/truthrange_latent_v0_2.cpp"
    "${MASTER_TILE}/streaming_scientific_master_tile_source_v0_1.cpp"
    "${TN}/truthnegative_continuous_v0_5.cpp"
    "${FREE_WORLD}/free_world_pixel_resolve_2d_v0_2.cpp"
    "${APP_CPP}/open_scene_channel_authority_v0_78.cpp"
    "${APP_CPP}/open_scene_field_v0_85.cpp"
    "${APP_CPP}/truthnegative_dense_local_field_adapter_v0_4.cpp"
    "${APP_CPP}/truthraw_sha256_v0_69.cpp"
    $<TARGET_OBJECTS:truthraw_v47i_frozen_core_authority_fusion>
)

target_include_directories(
    test_truthnegative_authority_field_fusion_v0_1 PRIVATE
    "${SMSB1}"
    "${SMSB2}"
    "${DIGEST}"
    "${TRUTHRANGE}"
    "${MASTER_TILE}"
    "${TN}"
    "${FREE_WORLD}"
    "${STREAM}"
    "${APP_CPP}"
    "${V47I}/include"
)

if(MSVC)
    target_compile_options(
        test_truthnegative_authority_field_fusion_v0_1
        PRIVATE /W4 /WX)
else()
    target_compile_options(
        test_truthnegative_authority_field_fusion_v0_1 PRIVATE
        -Wall -Wextra -Wpedantic -Werror
        -ffp-contract=off -fno-fast-math)
endif()

enable_testing()
add_test(
    NAME truthnegative_authority_field_fusion_v0_1
    COMMAND test_truthnegative_authority_field_fusion_v0_1)
