cmake_minimum_required(VERSION 3.16)
project(truthraw_libraw_compat_probe_v0_1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBRAW REQUIRED libraw)

add_executable(test_libraw_compat_probe_v0_1
  native/libraw_compat_probe_v0_1.cpp
  tests/test_libraw_compat_probe_v0_1.cpp)

target_include_directories(test_libraw_compat_probe_v0_1 PRIVATE
  native
  ../professional-raw-ingress-v0.1/native
  ${LIBRAW_INCLUDE_DIRS})
target_link_directories(test_libraw_compat_probe_v0_1 PRIVATE ${LIBRAW_LIBRARY_DIRS})
target_link_libraries(test_libraw_compat_probe_v0_1 PRIVATE ${LIBRAW_LIBRARIES})
target_compile_options(test_libraw_compat_probe_v0_1 PRIVATE
  -Wall -Wextra -Wpedantic -Werror ${LIBRAW_CFLAGS_OTHER})
