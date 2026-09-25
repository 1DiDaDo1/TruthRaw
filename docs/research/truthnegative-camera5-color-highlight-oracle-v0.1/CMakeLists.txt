cmake_minimum_required(VERSION 3.16)
project(truthnegative_camera5_color_highlight_oracle_v0_1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
get_filename_component(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
set(APP_CPP "${REPO_ROOT}/suite_android/app/src/main/cpp")
set(FREE_WORLD_V02 "${REPO_ROOT}/docs/research/free-world-pixel-resolve-2d-v0.2/native")
add_executable(test_truthnegative_camera5_color_highlight_oracle_v0_1
  tests/test_truthnegative_camera5_color_highlight_oracle_v0_1.cpp
  native/truthnegative_camera5_color_highlight_oracle_v0_1.cpp
  "${APP_CPP}/truthraw_sha256_v0_69.cpp")
target_include_directories(test_truthnegative_camera5_color_highlight_oracle_v0_1 PRIVATE
  native "${FREE_WORLD_V02}" "${APP_CPP}")
if(NOT MSVC)
 target_compile_options(test_truthnegative_camera5_color_highlight_oracle_v0_1 PRIVATE
  -Wall -Wextra -Wpedantic -Werror -ffp-contract=off -fno-fast-math)
endif()
enable_testing()
add_test(NAME truthnegative_camera5_color_highlight_oracle_v0_1
 COMMAND test_truthnegative_camera5_color_highlight_oracle_v0_1)
