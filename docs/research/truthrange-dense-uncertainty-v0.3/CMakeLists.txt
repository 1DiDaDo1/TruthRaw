cmake_minimum_required(VERSION 3.16)
project(truthraw_truthrange_dense_uncertainty_v0_3 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(TRUTHRAW_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../..")
set(V47I_INC "${TRUTHRAW_ROOT}/canonical/reconstruction/v4.7i/native/include")
set(V47I_CPP "${TRUTHRAW_ROOT}/canonical/reconstruction/v4.7i/native/src/core.cpp")
set(V5G_RT "${TRUTHRAW_ROOT}/canonical/uncertainty/v5.0g/runtime")
set(V02 "${TRUTHRAW_ROOT}/docs/research/truthrange-latent-binding-v0.2/native")
add_library(truthrange_dense_uncertainty_v0_3
  native/truthrange_dense_uncertainty_v0_3.cpp
  ${V02}/truthrange_latent_v0_2.cpp)
target_include_directories(truthrange_dense_uncertainty_v0_3 PUBLIC native ${V02} ${V47I_INC})
add_executable(test_truthrange_dense_uncertainty_v0_3
  tests/test_truthrange_dense_uncertainty_v0_3.cpp
  ${V47I_CPP}
  ${V5G_RT}/uncertainty_runtime_v5_0g.cpp)
target_include_directories(test_truthrange_dense_uncertainty_v0_3 PRIVATE native ${V02} ${V47I_INC} ${V5G_RT})
target_link_libraries(test_truthrange_dense_uncertainty_v0_3 PRIVATE truthrange_dense_uncertainty_v0_3)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(truthrange_dense_uncertainty_v0_3 PRIVATE -Wall -Wextra -Wpedantic -Werror)
  target_compile_options(test_truthrange_dense_uncertainty_v0_3 PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
