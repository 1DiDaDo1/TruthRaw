cmake_minimum_required(VERSION 3.16)
project(truthraw_precision_independent_v01 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)

set(TRUTHRAW_REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../..")
set(TRUTHRAW_V47I_INCLUDE "${TRUTHRAW_REPO_ROOT}/canonical/reconstruction/v4.7i/native/include")
set(TRUTHRAW_V47I_CORE "${TRUTHRAW_REPO_ROOT}/canonical/reconstruction/v4.7i/native/src/core.cpp")

add_library(truthraw_precision_v01 STATIC
    native/precision_policy_v0_1.cpp
    native/gainmap_precision_v0_2.cpp
    native/reconstruction_precision_reference_v0_3.cpp
    adapters/v47i_stage2_precision_adapter_v0_1.cpp
)
target_include_directories(truthraw_precision_v01 PUBLIC
    native
    adapters
    "${TRUTHRAW_V47I_INCLUDE}"
)

add_executable(test_precision_policy_v0_1
    native/test_precision_policy_v0_1.cpp
)
target_link_libraries(test_precision_policy_v0_1 PRIVATE truthraw_precision_v01)

add_executable(test_gainmap_precision_v0_2
    native/test_gainmap_precision_v0_2.cpp
)
target_link_libraries(test_gainmap_precision_v0_2 PRIVATE truthraw_precision_v01)

add_executable(test_v47i_stage2_precision_adapter_v0_1
    adapters/test_v47i_stage2_precision_adapter_v0_1.cpp
)
target_link_libraries(test_v47i_stage2_precision_adapter_v0_1 PRIVATE truthraw_precision_v01)

add_executable(test_reconstruction_precision_reference_v0_3
    native/test_reconstruction_precision_reference_v0_3.cpp
    "${TRUTHRAW_V47I_CORE}"
)
target_link_libraries(test_reconstruction_precision_reference_v0_3 PRIVATE
    truthraw_precision_v01
    Threads::Threads
)

add_executable(test_real_reconstruction_precision_fixture_v0_3
    native/test_real_reconstruction_precision_fixture_v0_3.cpp
)
target_link_libraries(test_real_reconstruction_precision_fixture_v0_3 PRIVATE truthraw_precision_v01)

enable_testing()
add_test(NAME precision_policy_v0_1 COMMAND test_precision_policy_v0_1)
add_test(NAME gainmap_precision_v0_2 COMMAND test_gainmap_precision_v0_2)
add_test(NAME v47i_stage2_precision_adapter_v0_1 COMMAND test_v47i_stage2_precision_adapter_v0_1)
add_test(NAME reconstruction_precision_reference_v0_3 COMMAND test_reconstruction_precision_reference_v0_3)
add_test(NAME real_reconstruction_precision_fixture_v0_3 COMMAND test_real_reconstruction_precision_fixture_v0_3)
