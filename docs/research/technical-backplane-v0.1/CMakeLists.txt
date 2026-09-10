cmake_minimum_required(VERSION 3.16)
project(truthraw_technical_backplane_v0_1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_executable(test_technical_backplane_v0_1
  native/technical_backplane_v0_1.cpp
  tests/test_technical_backplane_v0_1.cpp)
target_include_directories(test_technical_backplane_v0_1 PRIVATE native)
target_compile_options(test_technical_backplane_v0_1 PRIVATE -Wall -Wextra -Wpedantic -Werror)
