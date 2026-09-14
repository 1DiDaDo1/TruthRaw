cmake_minimum_required(VERSION 3.16)
project(truthraw_certificate_v0_1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(TRUTHRAW_CERT_V01_ENABLE_SANITIZERS "Enable ASan/UBSan" OFF)

add_executable(test_truthraw_certificate_v0_1
    tests/test_truthraw_certificate_v0_1.cpp
    native/truthraw_certificate_v0_1.cpp)

target_include_directories(test_truthraw_certificate_v0_1 PRIVATE native)

if(MSVC)
    target_compile_options(test_truthraw_certificate_v0_1 PRIVATE /W4 /WX)
else()
    target_compile_options(test_truthraw_certificate_v0_1 PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

if(TRUTHRAW_CERT_V01_ENABLE_SANITIZERS AND NOT MSVC)
    target_compile_options(test_truthraw_certificate_v0_1 PRIVATE
        -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(test_truthraw_certificate_v0_1 PRIVATE
        -fsanitize=address,undefined -fno-omit-frame-pointer)
endif()

enable_testing()
add_test(NAME truthraw_certificate_v0_1 COMMAND test_truthraw_certificate_v0_1)
