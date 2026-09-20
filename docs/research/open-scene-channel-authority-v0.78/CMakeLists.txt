cmake_minimum_required(VERSION 3.16)
project(truthraw_open_scene_channel_authority_v0_78 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT DEFINED TRUTHRAW_REPO_ROOT)
  get_filename_component(TRUTHRAW_REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
endif()
set(APP_CPP "${TRUTHRAW_REPO_ROOT}/suite_android/app/src/main/cpp")
set(STREAMING "${TRUTHRAW_REPO_ROOT}/docs/research/full-frame-streaming-v0.1/native")
set(V47I "${TRUTHRAW_REPO_ROOT}/canonical/reconstruction/v4.7i/native")

add_executable(test_open_scene_channel_authority_v0_78
    tests/test_open_scene_channel_authority_v0_78.cpp
    "${APP_CPP}/open_scene_channel_authority_v0_78.cpp"
    "${APP_CPP}/truthraw_sha256_v0_69.cpp"
)
target_include_directories(test_open_scene_channel_authority_v0_78 PRIVATE
    "${APP_CPP}" "${STREAMING}" "${V47I}/include")
target_compile_options(test_open_scene_channel_authority_v0_78 PRIVATE -Wall -Wextra -Wpedantic -Werror)
enable_testing()
add_test(NAME open_scene_channel_authority_v0_78 COMMAND test_open_scene_channel_authority_v0_78)
