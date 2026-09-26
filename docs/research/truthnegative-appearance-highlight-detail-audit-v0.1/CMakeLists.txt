cmake_minimum_required(VERSION 3.16)
project(truthnegative_appearance_highlight_detail_audit_v0_1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

get_filename_component(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
set(V01 "${CMAKE_CURRENT_SOURCE_DIR}/native")
set(FW "${REPO_ROOT}/docs/research/free-world-pixel-resolve-2d-v0.2/native")
set(OSF "${REPO_ROOT}/suite_android/app/src/main/cpp")
set(APP "${REPO_ROOT}/suite_android/app/src/main/cpp")

add_executable(test_truthnegative_appearance_highlight_detail_audit_v0_1
    tests/test_truthnegative_appearance_highlight_detail_audit_v0_1.cpp
    native/truthnegative_appearance_highlight_detail_audit_v0_1.cpp
    "${APP}/truthraw_sha256_v0_69.cpp"
)

target_include_directories(
    test_truthnegative_appearance_highlight_detail_audit_v0_1 PRIVATE
    "${V01}"
    "${FW}"
    "${OSF}"
    "${APP}"
)

if(MSVC)
    target_compile_options(
        test_truthnegative_appearance_highlight_detail_audit_v0_1
        PRIVATE /W4 /WX)
else()
    target_compile_options(
        test_truthnegative_appearance_highlight_detail_audit_v0_1 PRIVATE
        -Wall -Wextra -Wpedantic -Werror
        -ffp-contract=off -fno-fast-math)
endif()

enable_testing()
add_test(
    NAME truthnegative_appearance_highlight_detail_audit_v0_1
    COMMAND test_truthnegative_appearance_highlight_detail_audit_v0_1)
