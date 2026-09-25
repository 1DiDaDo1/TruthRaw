cmake_minimum_required(VERSION 3.16)
project(truthnegative_native_container_v0_1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
get_filename_component(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
set(APP_CPP "${REPO_ROOT}/suite_android/app/src/main/cpp")
set(TN_V05 "${REPO_ROOT}/docs/research/truthnegative-continuous-v0.5/native")
set(FW_V02 "${REPO_ROOT}/docs/research/free-world-pixel-resolve-2d-v0.2/native")
set(STREAMING "${REPO_ROOT}/docs/research/full-frame-streaming-v0.1/native")
set(V47I "${REPO_ROOT}/canonical/reconstruction/v4.7i/native")
add_executable(test_truthnegative_native_container_v0_1
 tests/test_truthnegative_native_container_v0_1.cpp
 native/truthnegative_native_container_v0_1.cpp
 "${TN_V05}/truthnegative_continuous_v0_5.cpp"
 "${FW_V02}/free_world_pixel_resolve_2d_v0_2.cpp"
 "${APP_CPP}/open_scene_channel_authority_v0_78.cpp"
 "${APP_CPP}/open_scene_field_v0_85.cpp"
 "${APP_CPP}/truthraw_sha256_v0_69.cpp")
target_include_directories(test_truthnegative_native_container_v0_1 PRIVATE
 native "${TN_V05}" "${FW_V02}" "${APP_CPP}" "${STREAMING}" "${V47I}/include")
if(NOT MSVC)
 target_compile_options(test_truthnegative_native_container_v0_1 PRIVATE
 -Wall -Wextra -Wpedantic -Werror -ffp-contract=off -fno-fast-math)
endif()
enable_testing()
add_test(NAME truthnegative_native_container_v0_1 COMMAND test_truthnegative_native_container_v0_1)
