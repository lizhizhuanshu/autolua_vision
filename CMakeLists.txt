cmake_minimum_required(VERSION 3.7.0)

project(alv)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")

option(VISION_SHARED "build shared library" ON)
option(VISION_STATIC "build static library" OFF)
option(BUILD_TESTING "build tests" ON)


find_package(OpenCV REQUIRED)

include_directories(${OpenCV_INCLUDE_DIRS} 
  ./src ./lodepng)

aux_source_directory(./src DIR_SRCS)
list(APPEND DIR_SRCS ./lodepng/lodepng.cpp)

if(VISION_SHARED)
  add_library(${PROJECT_NAME} SHARED ${DIR_SRCS})
  set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
  target_link_libraries(${PROJECT_NAME} ${OpenCV_LIBS})
endif()

if(VISION_STATIC)
  add_library(${PROJECT_NAME}_static STATIC ${DIR_SRCS})
  target_link_libraries(${PROJECT_NAME}_static ${OpenCV_LIBS})
endif()


if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()
