cmake_minimum_required(VERSION 3.16)
project(truthraw_manifold_conditioning_v1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(TRUTHRAW_V09_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../virtual-observation-manifold-v0.9")
foreach(req
    "${TRUTHRAW_V09_DIR}/CMakeLists.txt"
    "${TRUTHRAW_V09_DIR}/native/virtual_observation_manifold_v0_9.h")
  if(NOT EXISTS "${req}")
    message(FATAL_ERROR "Required closed TruthRaw v0.9 dependency missing: ${req}")
  endif()
endforeach()

add_subdirectory("${TRUTHRAW_V09_DIR}" "${CMAKE_BINARY_DIR}/virtual_observation_manifold_v09")

add_library(manifold_conditioning_v1 native/manifold_conditioning_v1.cpp)
target_include_directories(manifold_conditioning_v1 PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/native"
    "${TRUTHRAW_V09_DIR}/native")
target_link_libraries(manifold_conditioning_v1 PUBLIC virtual_observation_manifold_v0_9)

add_executable(test_manifold_conditioning_v1 tests/test_manifold_conditioning_v1.cpp)
target_link_libraries(test_manifold_conditioning_v1 PRIVATE manifold_conditioning_v1)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  foreach(t manifold_conditioning_v1 test_manifold_conditioning_v1)
    target_compile_options(${t} PRIVATE -Wall -Wextra -Wpedantic -Werror)
  endforeach()
endif()

enable_testing()
add_test(NAME manifold_conditioning_v1 COMMAND test_manifold_conditioning_v1)
