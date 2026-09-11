cmake_minimum_required(VERSION 3.22.1)
project(truthraw_validation_bridge LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

get_filename_component(REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../../../../.." ABSOLUTE)
set(RESEARCH_ROOT "${REPO_ROOT}/docs/research")
set(V47I_NATIVE "${REPO_ROOT}/canonical/reconstruction/v4.7i/native")

add_library(truthraw_validation_bridge SHARED
    validation_bridge.cpp
    ${RESEARCH_ROOT}/android-on-device-validation-v0.1/native/android_on_device_validation_v0_1.cpp
    ${RESEARCH_ROOT}/technical-backplane-v0.1/native/technical_backplane_v0_1.cpp)

target_include_directories(truthraw_validation_bridge PRIVATE
    ${RESEARCH_ROOT}/android-on-device-validation-v0.1/native
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

target_compile_options(truthraw_validation_bridge PRIVATE -Wall -Wextra -Werror)
