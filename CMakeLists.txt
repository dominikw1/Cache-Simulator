cmake_minimum_required(VERSION 3.17)
project(GRA_Cache)

set(SYSTEM_C_DIR ../systemc)
include_directories(${SYSTEM_C_DIR}/include/)
link_directories(${SYSTEM_C_DIR}/lib/)

add_compile_options(-Wall -Wextra -pedantic)

set(SANIZIER_COMPILE_OPTIONS -fsanitize=address -fsanitize=undefined -fsanitize=float-divide-by-zero -fsanitize=leak)
add_compile_options(${SANITIZER_COMPILE_OPTIONS})
add_link_options(${SANITIZER_COMPILE_OPTIONS})

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing()

set(SYSTEMC_HOME ${CMAKE_CURRENT_SOURCE_DIR}/../systemc)
message(SYSTEMC_HOME="${SYSTEMC_HOME}")

add_subdirectory(src)
add_subdirectory(tests)
