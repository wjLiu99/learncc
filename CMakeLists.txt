# 设置cmake的最低版本和项目名称
cmake_minimum_required(VERSION 3.0)
project(learn)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall ")

set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_CXX_STANDARD 20)
# include_directories(
#     ${PROJECT_SOURCE_DIR}/inc/test
#     ${PROJECT_SOURCE_DIR}/inc/threadpool
#     ${PROJECT_SOURCE_DIR}/inc/public
#     ${PROJECT_SOURCE_DIR}/inc/Interview
#     ${PROJECT_SOURCE_DIR}/inc/concurrent
#     # ${PROJECT_SOURCE_DIR}/inc/co_server
#     ${PROJECT_SOURCE_DIR}/inc/net_learn
#     ${PROJECT_SOURCE_DIR}/inc/gateserver
# )

link_directories(${PROJECT_SOURCE_DIR}/lib)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

# add_subdirectory(src/test)
# add_subdirectory(src/threadpool)
# add_subdirectory(src/concurrent)
# add_subdirectory(src/co_server)
# add_subdirectory(src/net_learn)
# add_subdirectory(src/gateserver)
# add_subdirectory(src/learn)
# add_subdirectory(src/c11)
add_subdirectory(src/mmuduo)
add_subdirectory(example)





# add_dependencies(test threadpool)