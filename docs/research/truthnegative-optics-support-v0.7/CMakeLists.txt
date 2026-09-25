cmake_minimum_required(VERSION 3.16)
project(truthnegative_optics_support_v0_7 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

get_filename_component(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
set(APP_CPP "${REPO_ROOT}/suite_android/app/src/main/cpp")
set(FREE_WORLD_V02 "${REPO_ROOT}/docs/research/free-world-pixel-resolve-2d-v0.2/native")

add_executable(test_truthnegative_optics_support_v0_7
    tests/test_truthnegative_optics_support_v0_7.cpp
    native/truthnegative_optics_support_v0_7.cpp
    "${APP_CPP}/truthraw_sha256_v0_69.cpp"
)

target_include_directories(test_truthnegative_optics_support_v0_7 PRIVATE
    native
    "${FREE_WORLD_V02}"
    "${APP_CPP}"
)

if(MSVC)
  target_compile_options(test_truthnegative_optics_support_v0_7 PRIVATE /W4 /WX)
else()
  target_compile_options(test_truthnegative_optics_support_v0_7 PRIVATE
      -Wall -Wextra -Wpedantic -Werror -ffp-contract=off -fno-fast-math)
endif()

enable_testing()
add_test(NAME truthnegative_optics_support_v0_7
         COMMAND test_truthnegative_optics_support_v0_7)
