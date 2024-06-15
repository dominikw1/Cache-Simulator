add_library(GRA_Cache_lib CacheInternal.cpp)
add_executable(GRA_Cache main.c)
target_link_libraries(GRA_Cache GRA_Cache_lib)