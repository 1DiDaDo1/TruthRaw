cmake_minimum_required(VERSION 3.16)
project(truthraw_professional_raw_decoder_adapter_v0_1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(test_professional_raw_decoder_adapter_v0_1
  native/professional_raw_decoder_adapter_v0_1.cpp
  ../professional-raw-ingress-v0.1/native/professional_raw_ingress_v0_1.cpp
  tests/test_professional_raw_decoder_adapter_v0_1.cpp)

target_include_directories(test_professional_raw_decoder_adapter_v0_1 PRIVATE
  native
  ../professional-raw-ingress-v0.1/native
  ../tile-native-dng-source-v0.1/native
  ../full-frame-streaming-v0.1/native
  ../../../canonical/reconstruction/v4.7i/native/include)

target_compile_options(test_professional_raw_decoder_adapter_v0_1 PRIVATE
  -Wall -Wextra -Wpedantic -Werror)
