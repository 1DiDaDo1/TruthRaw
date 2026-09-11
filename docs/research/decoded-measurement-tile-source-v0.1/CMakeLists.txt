cmake_minimum_required(VERSION 3.16)
project(truthraw_decoded_measurement_tile_source_v0_1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(test_decoded_measurement_tile_source_v0_1
  native/decoded_measurement_tile_source_v0_1.cpp
  ../decoded-measurement-handoff-v0.1/native/decoded_measurement_handoff_v0_1.cpp
  ../professional-raw-ingress-v0.1/native/professional_raw_ingress_v0_1.cpp
  tests/test_decoded_measurement_tile_source_v0_1.cpp)

target_include_directories(test_decoded_measurement_tile_source_v0_1 PRIVATE
  native
  ../decoded-measurement-handoff-v0.1/native
  ../professional-raw-ingress-v0.1/native
  ../full-frame-streaming-v0.1/native
  ../../../canonical/reconstruction/v4.7i/native/include)

target_compile_options(test_decoded_measurement_tile_source_v0_1 PRIVATE
  -Wall -Wextra -Wpedantic -Werror)
