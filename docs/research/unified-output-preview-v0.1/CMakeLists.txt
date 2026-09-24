cmake_minimum_required(VERSION 3.16)
project(truthraw_unified_output_preview_v01 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

get_filename_component(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
set(APP_CPP "${REPO_ROOT}/suite_android/app/src/main/cpp")
set(SCI_FLOAT "${REPO_ROOT}/docs/research/scientific-master-linear-dng-projection-v0.1/native")
set(TECH "${REPO_ROOT}/docs/research/technical-backplane-v0.1/native")
set(LATENT "${REPO_ROOT}/docs/research/truthrange-latent-binding-v0.2/native")

add_executable(test_unified_output_preview_v01
    tests/test_unified_output_preview_v01.cpp
    "${APP_CPP}/unified_output_preview_v0_1.cpp"
)

target_include_directories(test_unified_output_preview_v01 PRIVATE
    "${APP_CPP}"
    "${SCI_FLOAT}"
    "${TECH}"
    "${LATENT}"
)

if(MSVC)
    target_compile_options(test_unified_output_preview_v01 PRIVATE /W4 /WX)
else()
    target_compile_options(test_unified_output_preview_v01 PRIVATE
        -Wall -Wextra -Wpedantic -Werror
        -fno-fast-math
        -ffp-contract=off)
endif()

enable_testing()
add_test(NAME unified_output_preview_v01 COMMAND test_unified_output_preview_v01)
