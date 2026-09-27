cmake_minimum_required(VERSION 3.16)
project(free_world_observation_graph_v0_1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

get_filename_component(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
set(APP_CPP "${REPO_ROOT}/suite_android/app/src/main/cpp")

add_executable(test_free_world_observation_graph_v0_1
    tests/test_free_world_observation_graph_v0_1.cpp
    native/free_world_observation_graph_v0_1.cpp
    "${APP_CPP}/truthraw_sha256_v0_69.cpp"
)

target_include_directories(test_free_world_observation_graph_v0_1 PRIVATE
    native
    "${APP_CPP}"
)

if(MSVC)
  target_compile_options(test_free_world_observation_graph_v0_1 PRIVATE /W4 /WX)
else()
  target_compile_options(test_free_world_observation_graph_v0_1 PRIVATE
      -Wall -Wextra -Wpedantic -Werror -ffp-contract=off -fno-fast-math)
endif()

enable_testing()
add_test(NAME free_world_observation_graph_v0_1
         COMMAND test_free_world_observation_graph_v0_1)
