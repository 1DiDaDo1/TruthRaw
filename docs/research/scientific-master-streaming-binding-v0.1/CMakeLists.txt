cmake_minimum_required(VERSION 3.16)
project(truthraw_scientific_master_streaming_binding_v0_1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(TRUTHRAW_SMSB_ENABLE_SANITIZERS "Enable ASan/UBSan" OFF)

if(NOT DEFINED TRUTHRAW_REPO_ROOT)
  get_filename_component(TRUTHRAW_REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
endif()
set(V47I "${TRUTHRAW_REPO_ROOT}/canonical/reconstruction/v4.7i/native")

add_library(truthraw_v47i_frozen_core_smsb OBJECT "${V47I}/src/core.cpp")
target_include_directories(truthraw_v47i_frozen_core_smsb PRIVATE "${V47I}/include")
target_compile_options(truthraw_v47i_frozen_core_smsb PRIVATE -Wall -Wextra -Werror)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  target_compile_options(truthraw_v47i_frozen_core_smsb PRIVATE -Wno-error=misleading-indentation)
endif()

add_executable(test_scientific_master_streaming_binding_v0_1
    native/scientific_master_streaming_binding_v0_1.cpp
    tests/test_scientific_master_streaming_binding_v0_1.cpp
    ../scientific-master-digest-v0.1/native/scientific_master_digest_v0_1.cpp
    ../truthrange-latent-binding-v0.2/native/truthrange_latent_v0_2.cpp
    ../technical-backplane-phase2-v0.1/native/technical_backplane_phase2_v0_1.cpp
    ../technical-backplane-v0.1/native/technical_backplane_v0_1.cpp
    ../scientific-preview-source-binding-v0.1/native/scientific_preview_source_binding_v0_1.cpp
    ../scientific-preview-source-binding-v0.2/native/scientific_preview_source_binding_v0_2.cpp
    $<TARGET_OBJECTS:truthraw_v47i_frozen_core_smsb>
)

target_include_directories(test_scientific_master_streaming_binding_v0_1 PRIVATE
    native
    tests
    ../scientific-master-digest-v0.1/native
    ../truthrange-latent-binding-v0.2/native
    ../technical-backplane-phase2-v0.1/native
    ../technical-backplane-v0.1/native
    ../scientific-preview-source-binding-v0.1/native
    ../scientific-preview-source-binding-v0.2/native
    ../tile-native-dng-source-v0.1/native
    ../full-frame-streaming-v0.1/native
    ../full-frame-streaming-v0.1/tests
    "${V47I}/include"
)

if(MSVC)
    target_compile_options(test_scientific_master_streaming_binding_v0_1 PRIVATE /W4 /WX)
else()
    # The new module includes validated v0.1 streaming headers. Keep the same
    # strict warning baseline used by those modules rather than retroactively
    # imposing conversion warnings on historical header bytes.
    target_compile_options(test_scientific_master_streaming_binding_v0_1 PRIVATE
        -Wall -Wextra -Wpedantic -Werror)
endif()

if(TRUTHRAW_SMSB_ENABLE_SANITIZERS AND NOT MSVC)
    target_compile_options(test_scientific_master_streaming_binding_v0_1 PRIVATE
        -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(test_scientific_master_streaming_binding_v0_1 PRIVATE
        -fsanitize=address,undefined -fno-omit-frame-pointer)
endif()

enable_testing()
add_test(NAME scientific_master_streaming_binding_v0_1
         COMMAND test_scientific_master_streaming_binding_v0_1)
