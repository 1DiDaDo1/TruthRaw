cmake_minimum_required(VERSION 3.22)
project(truthraw_fotograaf_shadow_measurement_model_v0_1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)

add_library(fotograaf_shadow_measurement_model_v0_1 STATIC
    shadow_measurement_model_v0_1.cpp
)
target_include_directories(fotograaf_shadow_measurement_model_v0_1 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_compile_options(fotograaf_shadow_measurement_model_v0_1 PRIVATE
    -Wall -Wextra -Wpedantic -Werror
)

add_executable(test_fotograaf_shadow_measurement_model_v0_1
    test_shadow_measurement_model_v0_1.cpp
)
target_link_libraries(test_fotograaf_shadow_measurement_model_v0_1 PRIVATE
    fotograaf_shadow_measurement_model_v0_1
    Threads::Threads
)
target_compile_options(test_fotograaf_shadow_measurement_model_v0_1 PRIVATE
    -Wall -Wextra -Wpedantic -Werror
)

include(CTest)
add_test(NAME fotograaf_shadow_measurement_model_v0_1
         COMMAND test_fotograaf_shadow_measurement_model_v0_1)
