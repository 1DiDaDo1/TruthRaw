cmake_minimum_required(VERSION 3.16)
project(truthraw_hdr_authority_v0_83 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT DEFINED TRUTHRAW_REPO_ROOT)
  get_filename_component(TRUTHRAW_REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
endif()

set(APP_CPP "${TRUTHRAW_REPO_ROOT}/suite_android/app/src/main/cpp")

add_executable(test_hdr_authority_v0_83
  tests/test_hdr_authority_v0_83.cpp
  "${APP_CPP}/hdr_authority_v0_83.cpp"
  "${APP_CPP}/truthraw_sha256_v0_69.cpp"
)
target_include_directories(test_hdr_authority_v0_83 PRIVATE "${APP_CPP}")
target_compile_options(test_hdr_authority_v0_83 PRIVATE
  -Wall -Wextra -Wpedantic -Werror
)

enable_testing()
add_test(NAME hdr_authority_v0_83 COMMAND test_hdr_authority_v0_83)
