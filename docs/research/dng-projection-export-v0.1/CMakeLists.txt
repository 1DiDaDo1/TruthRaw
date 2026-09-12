cmake_minimum_required(VERSION 3.20)
project(truthraw_dng_projection_export_v0_1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()

get_filename_component(REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
set(RESEARCH_ROOT "${REPO_ROOT}/docs/research")
set(EXPORT_NATIVE "${CMAKE_CURRENT_LIST_DIR}/native")
set(STREAMING "${RESEARCH_ROOT}/full-frame-streaming-v0.1/native")
set(SCI_V01 "${RESEARCH_ROOT}/scientific-master-streaming-binding-v0.1/native")
set(SCI_V02 "${RESEARCH_ROOT}/scientific-master-streaming-binding-v0.2/native")
set(DIGEST "${RESEARCH_ROOT}/scientific-master-digest-v0.1/native")
set(SOURCE_V01 "${RESEARCH_ROOT}/scientific-preview-source-binding-v0.1/native")
set(SOURCE_V02 "${RESEARCH_ROOT}/scientific-preview-source-binding-v0.2/native")
set(BACKPLANE "${RESEARCH_ROOT}/technical-backplane-v0.1/native")
set(BACKPLANE_PHASE2 "${RESEARCH_ROOT}/technical-backplane-phase2-v0.1/native")
set(TRUTHRANGE "${RESEARCH_ROOT}/truthrange-latent-binding-v0.2/native")
set(TILE_DNG "${RESEARCH_ROOT}/tile-native-dng-source-v0.1/native")
set(V47I "${REPO_ROOT}/canonical/reconstruction/v4.7i/native")
set(TEST_SUPPORT "${RESEARCH_ROOT}/full-frame-streaming-v0.1/tests")

add_executable(test_dng_projection_export_v0_1
    tests/test_dng_projection_export_v0_1.cpp
    native/dng_projection_export_v0_1.cpp
    ${SCI_V01}/scientific_master_streaming_binding_v0_1.cpp
    ${SCI_V02}/scientific_master_streaming_binding_v0_2.cpp
    ${DIGEST}/scientific_master_digest_v0_1.cpp
    ${BACKPLANE}/technical_backplane_v0_1.cpp
    ${TRUTHRANGE}/truthrange_latent_v0_2.cpp
    ${V47I}/src/core.cpp)

target_include_directories(test_dng_projection_export_v0_1 PRIVATE
    ${EXPORT_NATIVE}
    ${STREAMING}
    ${SCI_V01}
    ${SCI_V02}
    ${DIGEST}
    ${SOURCE_V01}
    ${SOURCE_V02}
    ${BACKPLANE}
    ${BACKPLANE_PHASE2}
    ${TRUTHRANGE}
    ${TILE_DNG}
    ${V47I}/include
    ${TEST_SUPPORT})

target_compile_options(test_dng_projection_export_v0_1 PRIVATE -Wall -Wextra -Werror)
set_source_files_properties(
    ${V47I}/src/core.cpp
    PROPERTIES COMPILE_OPTIONS "-Wno-error=misleading-indentation")

add_test(NAME dng_projection_export_v0_1 COMMAND test_dng_projection_export_v0_1)
