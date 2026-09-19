cmake_minimum_required(VERSION 3.22.1)
project(truthraw_libraw_android_probe LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

# Pinned LibRaw 0.22.2 commit behind the signed release tag object.
set(TRUTHRAW_LIBRAW_COMMIT "b93f6e45c194f5df9b02a43b1af9a54b4f41f33f")
set(TRUTHRAW_LIBRAW_CMAKE_COMMIT "116924e5bcb9483ef9c5fbe7f3cfc5a8a4b97064")

FetchContent_Declare(
  truthraw_libraw_source
  GIT_REPOSITORY https://github.com/LibRaw/LibRaw.git
  GIT_TAG ${TRUTHRAW_LIBRAW_COMMIT}
  GIT_SHALLOW FALSE
)
FetchContent_GetProperties(truthraw_libraw_source)
if(NOT truthraw_libraw_source_POPULATED)
  FetchContent_Populate(truthraw_libraw_source)
endif()

FetchContent_Declare(
  truthraw_libraw_cmake
  GIT_REPOSITORY https://github.com/LibRaw/LibRaw-cmake.git
  GIT_TAG ${TRUTHRAW_LIBRAW_CMAKE_COMMIT}
  GIT_SHALLOW FALSE
)
FetchContent_GetProperties(truthraw_libraw_cmake)
if(NOT truthraw_libraw_cmake_POPULATED)
  FetchContent_Populate(truthraw_libraw_cmake)
endif()

set(LIBRAW_PATH "${truthraw_libraw_source_SOURCE_DIR}" CACHE STRING "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(ENABLE_OPENMP OFF CACHE BOOL "" FORCE)
set(ENABLE_LCMS OFF CACHE BOOL "" FORCE)
set(ENABLE_JASPER OFF CACHE BOOL "" FORCE)
set(ENABLE_EXAMPLES OFF CACHE BOOL "" FORCE)
set(ENABLE_RAWSPEED OFF CACHE BOOL "" FORCE)
set(ENABLE_DCRAW_DEBUG OFF CACHE BOOL "" FORCE)
set(ENABLE_X3FTOOLS OFF CACHE BOOL "" FORCE)
set(ENABLE_6BY9RPI OFF CACHE BOOL "" FORCE)
set(LIBRAW_INSTALL OFF CACHE BOOL "" FORCE)

add_subdirectory(
  "${truthraw_libraw_cmake_SOURCE_DIR}"
  "${CMAKE_BINARY_DIR}/libraw-cmake"
  EXCLUDE_FROM_ALL
)

add_library(truthraw_libraw_android_probe SHARED libraw_android_probe.cpp)
target_link_libraries(truthraw_libraw_android_probe PRIVATE libraw::libraw)
target_compile_options(truthraw_libraw_android_probe PRIVATE -Wall -Wextra -Werror)
