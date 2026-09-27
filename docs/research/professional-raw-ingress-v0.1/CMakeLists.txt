cmake_minimum_required(VERSION 3.16)
project(truthraw_professional_raw_ingress_v0_1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_executable(test_professional_raw_ingress_v0_1
  native/professional_raw_ingress_v0_1.cpp
  tests/test_professional_raw_ingress_v0_1.cpp)
target_include_directories(test_professional_raw_ingress_v0_1 PRIVATE native)
target_compile_options(test_professional_raw_ingress_v0_1 PRIVATE -Wall -Wextra -Wpedantic -Werror)
