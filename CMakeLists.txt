cmake_minimum_required(VERSION 3.17)
project(GRA_Cache)

add_compile_options(-Wall -Wextra -pedantic -Werror)

set(SANIZIER_COMPILE_OPTIONS -fsanitize=address -fsanitize=undefined -fsanitize=float-divide-by-zero -fsanitize=thread -fsanitize=leak)
add_compile_options(${SANITIZER_COMPILE_OPTIONS})
add_link_options(${SANITIZER_COMPILE_OPTIONS})

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing()

add_subdirectory(src)
add_subdirectory(tests)