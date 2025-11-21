cmake_minimum_required(VERSION 3.16)
project(task5_var20 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(pmr_queue INTERFACE)
target_include_directories(pmr_queue INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(pmr_queue INTERFACE cxx_std_20)

add_executable(main_demo main.cpp)
target_link_libraries(main_demo PRIVATE pmr_queue)

include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()
add_executable(queue_tests test.cpp)
target_link_libraries(queue_tests PRIVATE pmr_queue GTest::gtest_main)
include(GoogleTest)
gtest_discover_tests(queue_tests)
