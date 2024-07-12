include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        release-1.11.0
)
FetchContent_MakeAvailable(googletest)
include(GoogleTest)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_definitions(-g -Wall -Wextra -pedantic)


add_executable(tests template.cpp LRUTests.cpp CacheInternalTests.cpp Utils.cpp CPUTests.cpp FIFOTests.cpp CacheTests.cpp)
#target_link_libraries(tests  -lasan  -llsan -lubsan)



target_link_libraries(tests gtest_main gtest ${SYSTEMC_HOME}/lib/libsystemc.so GRA_Cache_lib)
gtest_discover_tests(tests)
