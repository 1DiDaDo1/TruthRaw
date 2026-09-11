cmake_minimum_required(VERSION 3.16)
project(truthraw_decoded_measurement_handoff_v0_1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(test_decoded_measurement_handoff_v0_1
  native/decoded_measurement_handoff_v0_1.cpp
  ../professional-raw-gatehouse-runtime-v0.1/native/professional_raw_gatehouse_runtime_v0_1.cpp
  ../professional-raw-decoder-adapter-v0.1/native/professional_raw_decoder_adapter_v0_1.cpp
  ../professional-raw-ingress-v0.1/native/professional_raw_ingress_v0_1.cpp
  ../tile-native-dng-source-v0.1/native/tile_native_dng_source_v0_1.cpp
  ../tile-native-dng-source-v0.1/native/tile_native_dng_source_v0_1_bind.cpp
  ../tile-native-dng-source-v0.1/native/tile_native_dng_source_v0_1_read.cpp
  tests/test_decoded_measurement_handoff_v0_1.cpp)

target_include_directories(test_decoded_measurement_handoff_v0_1 PRIVATE
  native
  ../professional-raw-gatehouse-runtime-v0.1/native
  ../professional-raw-decoder-adapter-v0.1/native
  ../professional-raw-ingress-v0.1/native
  ../tile-native-dng-source-v0.1/native
  ../full-frame-streaming-v0.1/native
  ../../../canonical/reconstruction/v4.7i/native/include)

target_compile_options(test_decoded_measurement_handoff_v0_1 PRIVATE
  -Wall -Wextra -Wpedantic -Werror)
