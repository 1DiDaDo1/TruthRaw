cmake_minimum_required(VERSION 3.16)
project(truthnegative_center_excluded_neighborhood_v0_2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(test_truthnegative_center_excluded_neighborhood_v0_2
    tests/test_truthnegative_center_excluded_neighborhood_v0_2.cpp
    native/truthnegative_center_excluded_neighborhood_v0_2.cpp
)

target_include_directories(test_truthnegative_center_excluded_neighborhood_v0_2 PRIVATE
    native
)

if(MSVC)
  target_compile_options(test_truthnegative_center_excluded_neighborhood_v0_2 PRIVATE /W4 /WX)
else()
  target_compile_options(test_truthnegative_center_excluded_neighborhood_v0_2 PRIVATE
      -Wall -Wextra -Wpedantic -Werror -ffp-contract=off -fno-fast-math)
endif()

enable_testing()
add_test(NAME truthnegative_center_excluded_neighborhood_v0_2
         COMMAND test_truthnegative_center_excluded_neighborhood_v0_2)
