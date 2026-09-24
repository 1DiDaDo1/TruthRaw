cmake_minimum_required(VERSION 3.16)
project(truthraw_open_scene_field_v085 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

get_filename_component(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
set(APP_CPP "${REPO_ROOT}/suite_android/app/src/main/cpp")
set(V47I "${REPO_ROOT}/canonical/reconstruction/v4.7i/native")
set(STREAMING "${REPO_ROOT}/docs/research/full-frame-streaming-v0.1/native")
set(SCIENTIFIC_MASTER_FLOAT_DNG "${REPO_ROOT}/docs/research/scientific-master-linear-dng-projection-v0.1/native")
set(TRUTHNEGATIVE_DENSE_V03 "${REPO_ROOT}/docs/research/truthnegative-dense-projection-v0.3/native")
set(TECHNICAL_BACKPLANE "${REPO_ROOT}/docs/research/technical-backplane-v0.1/native")
set(TRUTHRANGE_LATENT "${REPO_ROOT}/docs/research/truthrange-latent-binding-v0.2/native")

add_executable(test_open_scene_field_v085
    tests/test_open_scene_field_v085.cpp
    "${APP_CPP}/truthraw_sha256_v0_69.cpp"
    "${APP_CPP}/open_scene_field_v0_85.cpp"
    "${APP_CPP}/open_scene_local_policy_v0_86.cpp"
    "${APP_CPP}/truthnegative_local_authority_projection_v0_4.cpp"
    "${APP_CPP}/truthnegative_dense_local_field_adapter_v0_4.cpp"
)

target_include_directories(test_open_scene_field_v085 PRIVATE
    "${APP_CPP}"
    "${V47I}/include"
    "${STREAMING}"
    "${SCIENTIFIC_MASTER_FLOAT_DNG}"
    "${TRUTHNEGATIVE_DENSE_V03}"
    "${TECHNICAL_BACKPLANE}"
    "${TRUTHRANGE_LATENT}"
)

if(MSVC)
    target_compile_options(test_open_scene_field_v085 PRIVATE /W4 /WX)
else()
    target_compile_options(test_open_scene_field_v085 PRIVATE
        -Wall -Wextra -Wpedantic -Werror
        -fno-fast-math
        -ffp-contract=off)
endif()

enable_testing()
add_test(NAME open_scene_field_v085 COMMAND test_open_scene_field_v085)
