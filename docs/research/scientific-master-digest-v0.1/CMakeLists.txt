cmake_minimum_required(VERSION 3.16)
project(truthraw_scientific_master_digest_v0_1 LANGUAGES CXX)

option(TRUTHRAW_SMD_ENABLE_SANITIZERS "Enable Address/UndefinedBehavior sanitizers" OFF)

add_library(truthraw_scientific_master_digest_v0_1 STATIC
    native/scientific_master_digest_v0_1.cpp
)
target_include_directories(truthraw_scientific_master_digest_v0_1 PUBLIC native)
target_compile_features(truthraw_scientific_master_digest_v0_1 PUBLIC cxx_std_17)

if(MSVC)
    target_compile_options(truthraw_scientific_master_digest_v0_1 PRIVATE /W4 /WX)
else()
    target_compile_options(truthraw_scientific_master_digest_v0_1 PRIVATE
        -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror)
endif()

add_executable(test_scientific_master_digest_v0_1
    tests/test_scientific_master_digest_v0_1.cpp
)
target_link_libraries(test_scientific_master_digest_v0_1 PRIVATE
    truthraw_scientific_master_digest_v0_1
)
target_compile_features(test_scientific_master_digest_v0_1 PRIVATE cxx_std_17)
if(MSVC)
    target_compile_options(test_scientific_master_digest_v0_1 PRIVATE /W4 /WX)
else()
    target_compile_options(test_scientific_master_digest_v0_1 PRIVATE
        -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror)
endif()

if(TRUTHRAW_SMD_ENABLE_SANITIZERS AND NOT MSVC)
    foreach(target truthraw_scientific_master_digest_v0_1 test_scientific_master_digest_v0_1)
        target_compile_options(${target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(${target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    endforeach()
endif()

enable_testing()
add_test(NAME scientific_master_digest_v0_1 COMMAND test_scientific_master_digest_v0_1)
