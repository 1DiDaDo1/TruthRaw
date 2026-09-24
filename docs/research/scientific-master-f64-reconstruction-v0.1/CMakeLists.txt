cmake_minimum_required(VERSION 3.16)
project(truthraw_scientific_master_f64_reconstruction_v01 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(TRUTHRAW_F64_RECON_ENABLE_SANITIZERS "Enable ASan/UBSan" OFF)

get_filename_component(TRUTHRAW_REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
set(V47I "${TRUTHRAW_REPO_ROOT}/canonical/reconstruction/v4.7i/native")

add_executable(test_scientific_master_f64_reconstruction_v01
    tests/test_scientific_master_f64_reconstruction_v01.cpp
    "${V47I}/src/core.cpp"
)

target_include_directories(test_scientific_master_f64_reconstruction_v01 PRIVATE
    "${V47I}/include"
)

if(MSVC)
    target_compile_options(test_scientific_master_f64_reconstruction_v01 PRIVATE /W4 /WX)
else()
    target_compile_options(test_scientific_master_f64_reconstruction_v01 PRIVATE
        -Wall -Wextra -Wpedantic -Werror
        -fno-fast-math
        -ffp-contract=off
    )
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set_source_files_properties(
        "${V47I}/src/core.cpp"
        PROPERTIES COMPILE_OPTIONS "-Wno-error=misleading-indentation"
    )
endif()

if(TRUTHRAW_F64_RECON_ENABLE_SANITIZERS AND NOT MSVC)
    target_compile_options(test_scientific_master_f64_reconstruction_v01 PRIVATE
        -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(test_scientific_master_f64_reconstruction_v01 PRIVATE
        -fsanitize=address,undefined -fno-omit-frame-pointer)
endif()

enable_testing()
add_test(NAME scientific_master_f64_reconstruction_v01
         COMMAND test_scientific_master_f64_reconstruction_v01)
