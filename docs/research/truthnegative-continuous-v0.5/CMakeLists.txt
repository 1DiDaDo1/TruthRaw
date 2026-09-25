cmake_minimum_required(VERSION 3.16)
project(truthnegative_continuous_v0_5 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

get_filename_component(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
set(APP_CPP "${REPO_ROOT}/suite_android/app/src/main/cpp")
set(FREE_WORLD_V02 "${REPO_ROOT}/docs/research/free-world-pixel-resolve-2d-v0.2/native")

add_executable(test_truthnegative_continuous_v0_5
    tests/test_truthnegative_continuous_v0_5.cpp
    native/truthnegative_continuous_v0_5.cpp
    "${FREE_WORLD_V02}/free_world_pixel_resolve_2d_v0_2.cpp"
    "${APP_CPP}/open_scene_channel_authority_v0_78.cpp"
    "${APP_CPP}/open_scene_field_v0_85.cpp"
    "${APP_CPP}/truthraw_sha256_v0_69.cpp"
)

target_include_directories(test_truthnegative_continuous_v0_5 PRIVATE
    native
    "${FREE_WORLD_V02}"
    "${APP_CPP}"
)

if(MSVC)
  target_compile_options(test_truthnegative_continuous_v0_5 PRIVATE /W4 /WX)
else()
  target_compile_options(test_truthnegative_continuous_v0_5 PRIVATE
      -Wall -Wextra -Wpedantic -Werror -ffp-contract=off -fno-fast-math)
endif()

enable_testing()
add_test(NAME truthnegative_continuous_v0_5
         COMMAND test_truthnegative_continuous_v0_5)
