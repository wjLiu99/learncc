# 设置cmake的最低版本和项目名称
cmake_minimum_required(VERSION 3.0)
project(learn)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -fno-elide-constructors -std=c++17")

set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_CXX_STANDARD 17)
include_directories(
    ${PROJECT_SOURCE_DIR}/inc/test
    ${PROJECT_SOURCE_DIR}/inc/threadpool
    ${PROJECT_SOURCE_DIR}/inc/public
    ${PROJECT_SOURCE_DIR}/inc/Interview
)

link_directories(${PROJECT_SOURCE_DIR}/lib)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

add_subdirectory(src/test)
add_subdirectory(src/threadpool)


add_dependencies(test threadpool)