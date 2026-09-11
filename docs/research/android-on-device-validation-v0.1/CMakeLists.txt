cmake_minimum_required(VERSION 3.16)
project(truthraw_android_on_device_validation_v0_1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(RESEARCH_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/..")
set(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../..")
set(V47I_NATIVE "${REPO_ROOT}/canonical/reconstruction/v4.7i/native")

add_library(android_on_device_validation_v0_1
    native/android_on_device_validation_v0_1.cpp)

target_include_directories(android_on_device_validation_v0_1 PUBLIC
    native
    ${RESEARCH_ROOT}/room-abi-v0.2/native
    ${RESEARCH_ROOT}/building-runtime-v0.1/native
    ${RESEARCH_ROOT}/full-frame-streaming-v0.1/native
    ${RESEARCH_ROOT}/technical-backplane-v0.1/native
    ${RESEARCH_ROOT}/local-illumination-room-v0.2/native
    ${RESEARCH_ROOT}/counterfactual-illumination-capture-manifold-v1/native
    ${RESEARCH_ROOT}/local-illumination-room-capsule-v0.1/native
    ${RESEARCH_ROOT}/room-abi-v0.1/native
    ${RESEARCH_ROOT}/manifold-conditioning-v1/native
    ${V47I_NATIVE}/include)

add_executable(test_android_on_device_validation_v0_1
    tests/test_android_on_device_validation_v0_1.cpp
    ${RESEARCH_ROOT}/building-runtime-v0.1/native/building_runtime_v0_1.cpp
    ${RESEARCH_ROOT}/technical-backplane-v0.1/native/technical_backplane_v0_1.cpp)

target_link_libraries(test_android_on_device_validation_v0_1 PRIVATE
    android_on_device_validation_v0_1)

enable_testing()
add_test(NAME android_on_device_validation_v0_1
         COMMAND test_android_on_device_validation_v0_1)
