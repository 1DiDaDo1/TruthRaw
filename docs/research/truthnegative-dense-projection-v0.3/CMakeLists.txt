cmake_minimum_required(VERSION 3.16)
project(truthnegative_dense_projection_v0_3 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

get_filename_component(REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
set(PROJECTION "${REPO_ROOT}/docs/research/scientific-master-linear-dng-projection-v0.1/native")
set(DIGEST "${REPO_ROOT}/docs/research/scientific-master-digest-v0.1/native")
set(BACKPLANE "${REPO_ROOT}/docs/research/technical-backplane-v0.1/native")
set(TRUTHRANGE "${REPO_ROOT}/docs/research/truthrange-latent-binding-v0.2/native")
set(V47I "${REPO_ROOT}/canonical/reconstruction/v4.7i/native")

add_executable(test_truthnegative_dense_projection_v0_3
    tests/test_truthnegative_dense_projection_v0_3.cpp
    native/truthnegative_dense_projection_v0_3.cpp
    "${DIGEST}/scientific_master_digest_v0_1.cpp"
)

target_include_directories(test_truthnegative_dense_projection_v0_3 PRIVATE
    native
    "${PROJECTION}"
    "${DIGEST}"
    "${BACKPLANE}"
    "${TRUTHRANGE}"
    "${V47I}/include"
)

if(MSVC)
  target_compile_options(test_truthnegative_dense_projection_v0_3 PRIVATE /W4 /WX)
else()
  target_compile_options(test_truthnegative_dense_projection_v0_3 PRIVATE
      -Wall -Wextra -Wpedantic -Werror -ffp-contract=off -fno-fast-math)
endif()

enable_testing()
add_test(NAME truthnegative_dense_projection_v0_3
         COMMAND test_truthnegative_dense_projection_v0_3)
