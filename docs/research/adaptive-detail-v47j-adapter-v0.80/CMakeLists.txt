cmake_minimum_required(VERSION 3.16)
project(truthraw_adaptive_detail_v47j_adapter LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT DEFINED TRUTHRAW_REPO_ROOT)
  get_filename_component(TRUTHRAW_REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
endif()

set(APP_CPP "${TRUTHRAW_REPO_ROOT}/suite_android/app/src/main/cpp")
set(V47I "${TRUTHRAW_REPO_ROOT}/canonical/reconstruction/v4.7i/native")
set(V47J "${TRUTHRAW_REPO_ROOT}/canonical/detail/v4.7j/native")

add_library(v47j_reference_wrapper OBJECT
    tests/v47j_reference_wrapper.cpp)
target_include_directories(v47j_reference_wrapper PRIVATE
    "${V47J}/include")
target_compile_options(v47j_reference_wrapper PRIVATE
    -Wall -Wextra -Wpedantic -Werror)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  target_compile_options(v47j_reference_wrapper PRIVATE
      -Wno-error=misleading-indentation)
endif()

add_library(v47j_adapter OBJECT
    "${APP_CPP}/adaptive_detail_v47j_adapter.cpp")
target_include_directories(v47j_adapter PRIVATE
    "${APP_CPP}"
    "${V47I}/include")
target_compile_options(v47j_adapter PRIVATE
    -Wall -Wextra -Wpedantic -Werror)

add_executable(test_adaptive_detail_v47j_adapter
    tests/test_adaptive_detail_v47j_adapter.cpp
    $<TARGET_OBJECTS:v47j_adapter>
    $<TARGET_OBJECTS:v47j_reference_wrapper>
)
target_include_directories(test_adaptive_detail_v47j_adapter PRIVATE
    "${APP_CPP}"
    "${V47I}/include")
target_compile_options(test_adaptive_detail_v47j_adapter PRIVATE
    -Wall -Wextra -Wpedantic -Werror)

enable_testing()
add_test(NAME adaptive_detail_v47j_adapter
         COMMAND test_adaptive_detail_v47j_adapter)
