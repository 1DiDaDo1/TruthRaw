cmake_minimum_required(VERSION 3.16)
project(truthraw_worker_resource_invariance_v0_1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT DEFINED TRUTHRAW_REPO_ROOT)
  get_filename_component(TRUTHRAW_REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
endif()

set(V47I "${TRUTHRAW_REPO_ROOT}/canonical/reconstruction/v4.7i/native")

find_package(Threads REQUIRED)

# Compile the byte-frozen canonical v4.7i source without modifying it.
add_library(truthraw_v47i_frozen_core_for_worker_invariance OBJECT
  "${V47I}/src/core.cpp")
target_include_directories(truthraw_v47i_frozen_core_for_worker_invariance PRIVATE
  "${V47I}/include")
target_compile_options(truthraw_v47i_frozen_core_for_worker_invariance PRIVATE
  -Wall -Wextra -Werror)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  # Existing warning in frozen canonical source; do not mutate canonical bytes.
  target_compile_options(truthraw_v47i_frozen_core_for_worker_invariance PRIVATE
    -Wno-error=misleading-indentation)
endif()

add_executable(test_worker_resource_invariance_v0_1
  test_worker_resource_invariance_v0_1.cpp
  $<TARGET_OBJECTS:truthraw_v47i_frozen_core_for_worker_invariance>)
target_include_directories(test_worker_resource_invariance_v0_1 PRIVATE
  "${V47I}/include")
target_compile_options(test_worker_resource_invariance_v0_1 PRIVATE
  -Wall -Wextra -Werror)
target_link_libraries(test_worker_resource_invariance_v0_1 PRIVATE Threads::Threads)

enable_testing()
add_test(NAME worker_resource_invariance_v0_1
  COMMAND test_worker_resource_invariance_v0_1)
