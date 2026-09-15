cmake_minimum_required(VERSION 3.16)
project(truthraw_precision_independent_v01 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(truthraw_precision_v01 STATIC
    native/precision_policy_v0_1.cpp
)
target_include_directories(truthraw_precision_v01 PUBLIC native)

add_executable(test_precision_policy_v0_1
    native/test_precision_policy_v0_1.cpp
)
target_link_libraries(test_precision_policy_v0_1 PRIVATE truthraw_precision_v01)

enable_testing()
add_test(NAME precision_policy_v0_1 COMMAND test_precision_policy_v0_1)
