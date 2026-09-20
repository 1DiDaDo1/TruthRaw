cmake_minimum_required(VERSION 3.16)
project(truthraw_output_acutance_v0_81 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT DEFINED TRUTHRAW_REPO_ROOT)
  get_filename_component(TRUTHRAW_REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
endif()

set(APP_CPP "${TRUTHRAW_REPO_ROOT}/suite_android/app/src/main/cpp")
set(V47K "${TRUTHRAW_REPO_ROOT}/canonical/output-acutance/v4.7k/native")

add_executable(test_output_acutance_v0_81
  tests/test_output_acutance_v0_81.cpp
  "${APP_CPP}/output_acutance_v0_81.cpp"
  "${V47K}/output_acutance.cpp"
)
target_include_directories(test_output_acutance_v0_81 PRIVATE
  "${APP_CPP}"
  "${V47K}"
)
target_compile_options(test_output_acutance_v0_81 PRIVATE
  -Wall -Wextra -Wpedantic -Werror
)

enable_testing()
add_test(NAME output_acutance_v0_81 COMMAND test_output_acutance_v0_81)
