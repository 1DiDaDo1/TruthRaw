cmake_minimum_required(VERSION 3.20)
project(truthraw_linear_dng_projection_v0_1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()

get_filename_component(REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
set(RESEARCH_ROOT "${REPO_ROOT}/docs/research")
set(V47I_NATIVE "${REPO_ROOT}/canonical/reconstruction/v4.7i/native")
set(MODULE_NATIVE "${CMAKE_CURRENT_LIST_DIR}/native")
set(STREAMING "${RESEARCH_ROOT}/full-frame-streaming-v0.1/native")
set(TILE_DNG "${RESEARCH_ROOT}/tile-native-dng-source-v0.1/native")
set(FINALIZED_V02 "${RESEARCH_ROOT}/finalized-scientific-preview-release-v0.2/native")
set(PREVIEW_SURFACE "${RESEARCH_ROOT}/reconstructed-color-preview-v0.1/native")
set(SCI_MASTER_V01 "${RESEARCH_ROOT}/scientific-master-streaming-binding-v0.1/native")
set(SCI_MASTER_V02 "${RESEARCH_ROOT}/scientific-master-streaming-binding-v0.2/native")
set(SCI_DIGEST "${RESEARCH_ROOT}/scientific-master-digest-v0.1/native")
set(TRUTHRANGE "${RESEARCH_ROOT}/truthrange-latent-binding-v0.2/native")
set(BACKPLANE "${RESEARCH_ROOT}/technical-backplane-v0.1/native")
set(BACKPLANE_PHASE2 "${RESEARCH_ROOT}/technical-backplane-phase2-v0.1/native")
set(SOURCE_BIND_V01 "${RESEARCH_ROOT}/scientific-preview-source-binding-v0.1/native")
set(SOURCE_BIND_V02 "${RESEARCH_ROOT}/scientific-preview-source-binding-v0.2/native")

add_executable(test_linear_dng_projection_v0_1
    native/linear_dng_projection_v0_1.cpp
    tests/test_linear_dng_projection_v0_1.cpp)

target_include_directories(test_linear_dng_projection_v0_1 PRIVATE
    ${MODULE_NATIVE}
    ${STREAMING}
    ${TILE_DNG}
    ${FINALIZED_V02}
    ${PREVIEW_SURFACE}
    ${SCI_MASTER_V01}
    ${SCI_MASTER_V02}
    ${SCI_DIGEST}
    ${TRUTHRANGE}
    ${BACKPLANE}
    ${BACKPLANE_PHASE2}
    ${SOURCE_BIND_V01}
    ${SOURCE_BIND_V02}
    ${V47I_NATIVE}/include)

target_compile_options(test_linear_dng_projection_v0_1 PRIVATE -Wall -Wextra -Werror)

add_test(NAME linear_dng_projection_v0_1 COMMAND test_linear_dng_projection_v0_1)
