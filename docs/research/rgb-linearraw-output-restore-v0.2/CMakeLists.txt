cmake_minimum_required(VERSION 3.20)
project(truthraw_rgb_linearraw_restore_v0_2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(TRUTHRAW_RGB_LINEARRAW_SANITIZERS "Enable ASan/UBSan" OFF)

add_executable(test_rgb_linearraw_compat_v0_2
    tests/test_rgb_linearraw_compat_v0_2.cpp
    native/rgb_linearraw_compat_v0_2.cpp)

target_include_directories(test_rgb_linearraw_compat_v0_2 PRIVATE native)
target_compile_options(test_rgb_linearraw_compat_v0_2 PRIVATE -Wall -Wextra -Werror)

if(TRUTHRAW_RGB_LINEARRAW_SANITIZERS AND NOT MSVC)
    target_compile_options(test_rgb_linearraw_compat_v0_2 PRIVATE
        -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(test_rgb_linearraw_compat_v0_2 PRIVATE
        -fsanitize=address,undefined -fno-omit-frame-pointer)
endif()

enable_testing()
add_test(NAME rgb_linearraw_compat_v0_2 COMMAND test_rgb_linearraw_compat_v0_2)
