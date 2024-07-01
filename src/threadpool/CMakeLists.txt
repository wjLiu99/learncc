project(threadpool)
file(GLOB_RECURSE SRC_LIST "*.cc")


find_package(Threads REQUIRED)
add_library(${PROJECT_NAME} SHARED ${SRC_LIST})
target_link_libraries(${PROJECT_NAME} Threads::Threads)
