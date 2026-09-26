cmake_minimum_required(VERSION 3.16)
project(truthnegative_n2_confidence_field_v0_3 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

get_filename_component(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
set(V03 "${CMAKE_CURRENT_SOURCE_DIR}/native")
set(V021 "${REPO_ROOT}/docs/research/truthnegative-center-excluded-spatial-audit-v0.2.1/native")
set(V01 "${REPO_ROOT}/docs/research/truthnegative-n2-cfa-audit-v0.1/native")
set(APP "${REPO_ROOT}/suite_android/app/src/main/cpp")
set(STREAM "${REPO_ROOT}/docs/research/full-frame-streaming-v0.1/native")
set(PIPE "${REPO_ROOT}/docs/research/truthnegative-n2-candidate-pipeline-v0.1/native")
set(GATE "${REPO_ROOT}/docs/research/truthnegative-structure-preservation-gate-v0.1/native")
set(NEIGH "${REPO_ROOT}/docs/research/truthnegative-authority-aware-neighborhood-v0.1/native")
set(RESID "${REPO_ROOT}/docs/research/truthnegative-bounded-residual-estimator-v0.1/native")
set(CE "${REPO_ROOT}/docs/research/truthnegative-center-excluded-neighborhood-v0.2/native")
set(V47I "${REPO_ROOT}/canonical/reconstruction/v4.7i/native")

add_executable(test_truthnegative_n2_confidence_field_v0_3
    tests/test_truthnegative_n2_confidence_field_v0_3.cpp
    native/truthnegative_n2_confidence_field_v0_3.cpp
    "${APP}/truthraw_sha256_v0_69.cpp"
)

target_include_directories(
    test_truthnegative_n2_confidence_field_v0_3 PRIVATE
    "${V03}"
    "${V021}"
    "${V01}"
    "${APP}"
    "${STREAM}"
    "${PIPE}"
    "${GATE}"
    "${NEIGH}"
    "${RESID}"
    "${CE}"
    "${V47I}/include"
)

if(MSVC)
    target_compile_options(
        test_truthnegative_n2_confidence_field_v0_3 PRIVATE /W4 /WX)
else()
    target_compile_options(
        test_truthnegative_n2_confidence_field_v0_3 PRIVATE
        -Wall -Wextra -Wpedantic -Werror
        -ffp-contract=off -fno-fast-math)
endif()

enable_testing()
add_test(
    NAME truthnegative_n2_confidence_field_v0_3
    COMMAND test_truthnegative_n2_confidence_field_v0_3)
