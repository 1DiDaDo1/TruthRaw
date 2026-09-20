cmake_minimum_required(VERSION 3.16)
project(truthraw_canonical_ancestry_v0_77 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT DEFINED TRUTHRAW_REPO_ROOT)
  get_filename_component(TRUTHRAW_REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
endif()
set(APP_CPP "${TRUTHRAW_REPO_ROOT}/suite_android/app/src/main/cpp")
set(BACKPLANE "${TRUTHRAW_REPO_ROOT}/docs/research/technical-backplane-v0.1/native")

add_executable(test_canonical_ancestry_v0_77
    tests/test_canonical_ancestry_v0_77.cpp
    "${APP_CPP}/canonical_ancestry_v0_77.cpp"
    "${APP_CPP}/truthraw_sha256_v0_69.cpp"
    "${BACKPLANE}/technical_backplane_v0_1.cpp"
)
target_include_directories(test_canonical_ancestry_v0_77 PRIVATE
    "${APP_CPP}" "${BACKPLANE}")
target_compile_options(test_canonical_ancestry_v0_77 PRIVATE -Wall -Wextra -Wpedantic -Werror)
enable_testing()
add_test(NAME canonical_ancestry_v0_77 COMMAND test_canonical_ancestry_v0_77)
