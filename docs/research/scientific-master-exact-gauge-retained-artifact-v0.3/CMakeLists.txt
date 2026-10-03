cmake_minimum_required(VERSION 3.16)
project(draw_scientific_master_exact_gauge_retained_artifact_v0_3 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(DRAW_EGA3_ENABLE_SANITIZERS "Enable ASan/UBSan" OFF)

if(NOT DEFINED DRAW_REPO_ROOT)
  get_filename_component(DRAW_REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
endif()
set(V47I "${DRAW_REPO_ROOT}/canonical/reconstruction/v4.7i/native")

add_library(draw_v47i_frozen_core_ega3 OBJECT "${V47I}/src/core.cpp")
target_include_directories(draw_v47i_frozen_core_ega3 PRIVATE "${V47I}/include")
target_compile_options(draw_v47i_frozen_core_ega3 PRIVATE -Wall -Wextra -Werror)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  target_compile_options(draw_v47i_frozen_core_ega3 PRIVATE -Wno-error=misleading-indentation)
endif()

add_executable(test_scientific_master_exact_gauge_retained_artifact_v0_3
    native/scientific_master_exact_gauge_retained_artifact_v0_3.cpp
    native/scientific_master_exact_gauge_retained_artifact_v0_3_admission.cpp
    tests/test_scientific_master_exact_gauge_retained_artifact_v0_3.cpp
    ../scientific-master-streaming-binding-v0.2/native/scientific_master_streaming_binding_v0_2.cpp
    ../scientific-master-streaming-binding-v0.1/native/scientific_master_streaming_binding_v0_1.cpp
    ../scientific-master-digest-v0.1/native/scientific_master_digest_v0_1.cpp
    ../truthrange-latent-binding-v0.2/native/truthrange_latent_v0_2.cpp
    $<TARGET_OBJECTS:draw_v47i_frozen_core_ega3>
)

target_include_directories(test_scientific_master_exact_gauge_retained_artifact_v0_3 PRIVATE
    native
    tests
    ../scientific-master-streaming-binding-v0.2/native
    ../scientific-master-streaming-binding-v0.1/native
    ../scientific-master-digest-v0.1/native
    ../truthrange-latent-binding-v0.2/native
    ../tile-native-dng-source-v0.1/native
    ../full-frame-streaming-v0.1/native
    ../full-frame-streaming-v0.1/tests
    "${V47I}/include"
)

if(MSVC)
    target_compile_options(test_scientific_master_exact_gauge_retained_artifact_v0_3 PRIVATE /W4 /WX)
else()
    target_compile_options(test_scientific_master_exact_gauge_retained_artifact_v0_3 PRIVATE
        -Wall -Wextra -Wpedantic -Werror)
endif()

if(DRAW_EGA3_ENABLE_SANITIZERS AND NOT MSVC)
    target_compile_options(test_scientific_master_exact_gauge_retained_artifact_v0_3 PRIVATE
        -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(test_scientific_master_exact_gauge_retained_artifact_v0_3 PRIVATE
        -fsanitize=address,undefined -fno-omit-frame-pointer)
endif()

enable_testing()
add_test(NAME scientific_master_exact_gauge_retained_artifact_v0_3
         COMMAND test_scientific_master_exact_gauge_retained_artifact_v0_3)
