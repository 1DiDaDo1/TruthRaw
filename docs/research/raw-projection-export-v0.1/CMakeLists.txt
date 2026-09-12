cmake_minimum_required(VERSION 3.20)
project(truthraw_raw_projection_export_v0_1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

get_filename_component(REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
set(RESEARCH_ROOT "${REPO_ROOT}/docs/research")
set(V47I_NATIVE "${REPO_ROOT}/canonical/reconstruction/v4.7i/native")
set(STREAMING "${RESEARCH_ROOT}/full-frame-streaming-v0.1/native")
set(SCI_MASTER_V01 "${RESEARCH_ROOT}/scientific-master-streaming-binding-v0.1/native")
set(SCI_MASTER_V02 "${RESEARCH_ROOT}/scientific-master-streaming-binding-v0.2/native")
set(SCI_DIGEST "${RESEARCH_ROOT}/scientific-master-digest-v0.1/native")
set(TRUTHRANGE "${RESEARCH_ROOT}/truthrange-latent-binding-v0.2/native")

add_executable(test_raw_projection_export_v0_1
    tests/test_raw_projection_export_v0_1.cpp
    native/raw_projection_export_v0_1.cpp
    ${SCI_MASTER_V01}/scientific_master_streaming_binding_v0_1.cpp
    ${SCI_MASTER_V02}/scientific_master_streaming_binding_v0_2.cpp
    ${SCI_DIGEST}/scientific_master_digest_v0_1.cpp
    ${TRUTHRANGE}/truthrange_latent_v0_2.cpp
    ${V47I_NATIVE}/src/core.cpp)

target_include_directories(test_raw_projection_export_v0_1 PRIVATE
    native
    ${RESEARCH_ROOT}/full-frame-streaming-v0.1/tests
    ${STREAMING}
    ${SCI_MASTER_V01}
    ${SCI_MASTER_V02}
    ${SCI_DIGEST}
    ${TRUTHRANGE}
    ${V47I_NATIVE}/include)

target_compile_options(test_raw_projection_export_v0_1 PRIVATE -Wall -Wextra -Werror)
set_source_files_properties(
    ${V47I_NATIVE}/src/core.cpp
    PROPERTIES COMPILE_OPTIONS "-Wno-error=misleading-indentation")

enable_testing()
add_test(NAME raw_projection_export_v0_1 COMMAND test_raw_projection_export_v0_1)
