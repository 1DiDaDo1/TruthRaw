cmake_minimum_required(VERSION 3.16)
project(truthraw_technical_backplane_phase2_v0_1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(TRUTHRAW_TBP2_ENABLE_SANITIZERS "Enable ASan/UBSan" OFF)

add_executable(test_technical_backplane_phase2_v0_1
    native/technical_backplane_phase2_v0_1.cpp
    tests/test_technical_backplane_phase2_v0_1.cpp
    ../technical-backplane-v0.1/native/technical_backplane_v0_1.cpp
    ../scientific-preview-source-binding-v0.1/native/scientific_preview_source_binding_v0_1.cpp
    ../scientific-preview-source-binding-v0.2/native/scientific_preview_source_binding_v0_2.cpp
    ../scientific-master-digest-v0.1/native/scientific_master_digest_v0_1.cpp
)

target_include_directories(test_technical_backplane_phase2_v0_1 PRIVATE
    native
    ../technical-backplane-v0.1/native
    ../scientific-preview-source-binding-v0.1/native
    ../scientific-preview-source-binding-v0.2/native
    ../scientific-master-digest-v0.1/native
    ../truthrange-latent-binding-v0.2/native
    ../tile-native-dng-source-v0.1/native
    ../full-frame-streaming-v0.1/native
    ../../../canonical/reconstruction/v4.7i/native/include
)

if(MSVC)
    target_compile_options(test_technical_backplane_phase2_v0_1 PRIVATE /W4 /WX)
else()
    # Existing validated modules retain the warning profile under which their
    # own CI was proved. The newly added phase-2 sources get the stricter
    # conversion diagnostics without forcing historical bytes to change.
    target_compile_options(test_technical_backplane_phase2_v0_1 PRIVATE
        -Wall -Wextra -Wpedantic -Werror)
    set_source_files_properties(
        native/technical_backplane_phase2_v0_1.cpp
        tests/test_technical_backplane_phase2_v0_1.cpp
        PROPERTIES COMPILE_OPTIONS "-Wconversion;-Wsign-conversion"
    )
endif()

if(TRUTHRAW_TBP2_ENABLE_SANITIZERS AND NOT MSVC)
    target_compile_options(test_technical_backplane_phase2_v0_1 PRIVATE
        -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(test_technical_backplane_phase2_v0_1 PRIVATE
        -fsanitize=address,undefined -fno-omit-frame-pointer)
endif()

enable_testing()
add_test(NAME technical_backplane_phase2_v0_1 COMMAND test_technical_backplane_phase2_v0_1)
