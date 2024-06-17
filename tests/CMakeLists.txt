
cmake_minimum_required(VERSION 3.7.0)

set(CMAKE_CXX_STANDARD 17)

find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

add_executable(tests test_encode.cc ../src/vision_color.cc  ../src/vision_feature.cc)
target_link_libraries(tests ${GTEST_LIBRARIES})




gtest_discover_tests(tests)