cmake_minimum_required(VERSION 3.17)
project(GRA_Cache)

set(SYSTEMC_HOME $ENV{SYSTEMC_HOME})
message(SYSTEMC_HOME="${SYSTEMC_HOME}")
include_directories(${SYSTEMC_HOME}/include/)
link_directories(${SYSTEMC_HOME}/lib/)

add_compile_options(-Wall -Wextra -pedantic)

set(SANIZIER_COMPILE_OPTIONS -fsanitize=address -fsanitize=undefined -fsanitize=float-divide-by-zero -fsanitize=leak)
add_compile_options(${SANITIZER_COMPILE_OPTIONS})
add_link_options(${SANITIZER_COMPILE_OPTIONS})


add_compile_definitions(RAM_DEBUG)
if (BUILD_INTEGRATION_TESTING) 
    add_compile_definitions(DEBUG_RUN_TILL_END)
endif()

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing()

add_subdirectory(src/Simulation)
add_subdirectory(tests)
