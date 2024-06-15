include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        release-1.11.0
)
FetchContent_MakeAvailable(googletest)
include(GoogleTest)

add_executable(tests template.cpp LRUTests.cpp CacheInternalTests.cpp Utils.cpp)
target_link_libraries(tests gtest_main gtest GRA_Cache_lib)
gtest_discover_tests(tests)
