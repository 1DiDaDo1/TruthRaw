cmake_minimum_required(VERSION 3.16)
project(truthnegative_n2_full_colour_candidate_v0_1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

get_filename_component(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
set(F64 "${REPO_ROOT}/docs/research/scientific-master-f64-reconstruction-v0.1/native")
set(CFA "${REPO_ROOT}/docs/research/truthnegative-n2-cfa-audit-v0.1/native")
set(PIPE "${REPO_ROOT}/docs/research/truthnegative-n2-candidate-pipeline-v0.1/native")
set(GATE "${REPO_ROOT}/docs/research/truthnegative-structure-preservation-gate-v0.1/native")
set(NEIGH "${REPO_ROOT}/docs/research/truthnegative-authority-aware-neighborhood-v0.1/native")
set(RESID "${REPO_ROOT}/docs/research/truthnegative-bounded-residual-estimator-v0.1/native")
set(STREAM "${REPO_ROOT}/docs/research/full-frame-streaming-v0.1/native")
set(APP "${REPO_ROOT}/suite_android/app/src/main/cpp")
set(V47I "${REPO_ROOT}/canonical/reconstruction/v4.7i/native")

add_executable(test_truthnegative_n2_full_colour_candidate_v0_1
 tests/test_truthnegative_n2_full_colour_candidate_v0_1.cpp
 native/truthnegative_n2_full_colour_candidate_v0_1.cpp
 "${F64}/scientific_master_f64_reconstruction_v0_1.cpp"
 "${APP}/truthraw_sha256_v0_69.cpp"
 "${V47I}/src/core.cpp"
)

target_include_directories(test_truthnegative_n2_full_colour_candidate_v0_1 PRIVATE
 native "${F64}" "${CFA}" "${PIPE}" "${GATE}" "${NEIGH}" "${RESID}"
 "${STREAM}" "${APP}" "${V47I}/include"
)

if(MSVC)
 target_compile_options(test_truthnegative_n2_full_colour_candidate_v0_1 PRIVATE /W4 /WX)
else()
 target_compile_options(test_truthnegative_n2_full_colour_candidate_v0_1 PRIVATE
   -Wall -Wextra -Wpedantic -Werror -ffp-contract=off -fno-fast-math)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
 set_source_files_properties(
   "${V47I}/src/core.cpp"
   PROPERTIES COMPILE_OPTIONS "-Wno-error=misleading-indentation")
endif()

enable_testing()
add_test(NAME truthnegative_n2_full_colour_candidate_v0_1
 COMMAND test_truthnegative_n2_full_colour_candidate_v0_1)
