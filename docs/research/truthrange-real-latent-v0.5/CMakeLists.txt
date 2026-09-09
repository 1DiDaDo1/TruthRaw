cmake_minimum_required(VERSION 3.16)
project(truthraw_truthrange_real_latent_v0_5 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(TRUTHRAW_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../..")
set(V47I_INC "${TRUTHRAW_ROOT}/canonical/reconstruction/v4.7i/native/include")
set(V47I_CPP "${TRUTHRAW_ROOT}/canonical/reconstruction/v4.7i/native/src/core.cpp")
set(V5G_RT "${TRUTHRAW_ROOT}/canonical/uncertainty/v5.0g/runtime")
set(V02 "${TRUTHRAW_ROOT}/docs/research/truthrange-latent-binding-v0.2/native")
set(V03 "${TRUTHRAW_ROOT}/docs/research/truthrange-dense-uncertainty-v0.3/native")
set(V04 "${TRUTHRAW_ROOT}/docs/research/truthrange-real-dng-stage2-v0.4/native")
add_library(truthrange_real_latent_v0_5
  native/truthrange_real_latent_v0_5.cpp
  ${V02}/truthrange_latent_v0_2.cpp
  ${V04}/dng_stage2_v0_4.cpp)
target_include_directories(truthrange_real_latent_v0_5 PUBLIC native ${V02} ${V03} ${V04} ${V47I_INC} ${V5G_RT})
add_executable(test_truthrange_real_latent_v0_5
  tests/test_truthrange_real_latent_v0_5.cpp
  ${V47I_CPP}
  ${V5G_RT}/uncertainty_runtime_v5_0g.cpp)
target_link_libraries(test_truthrange_real_latent_v0_5 PRIVATE truthrange_real_latent_v0_5)
target_include_directories(test_truthrange_real_latent_v0_5 PRIVATE native ${V02} ${V03} ${V04} ${V47I_INC} ${V5G_RT})
add_executable(truthrange_real_latent_v0_5_cli
  native/truthrange_real_latent_v0_5_cli.cpp
  ${V47I_CPP}
  ${V5G_RT}/uncertainty_runtime_v5_0g.cpp)
target_link_libraries(truthrange_real_latent_v0_5_cli PRIVATE truthrange_real_latent_v0_5)
target_include_directories(truthrange_real_latent_v0_5_cli PRIVATE native ${V02} ${V03} ${V04} ${V47I_INC} ${V5G_RT})
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  foreach(t truthrange_real_latent_v0_5 test_truthrange_real_latent_v0_5 truthrange_real_latent_v0_5_cli)
    target_compile_options(${t} PRIVATE -Wall -Wextra -Wpedantic -Werror)
  endforeach()
endif()
