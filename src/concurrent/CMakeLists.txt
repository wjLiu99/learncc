project(concurrent)
file(GLOB_RECURSE SRC_LIST "*.cc")


find_package(Threads REQUIRED)
add_executable(${PROJECT_NAME} ${SRC_LIST})
target_link_libraries(${PROJECT_NAME} Threads::Threads threadpool)
