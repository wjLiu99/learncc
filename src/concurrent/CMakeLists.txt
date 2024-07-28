project(concurrent)
file(GLOB_RECURSE SRC_LIST "*.cc")


include_directories(

    ${CMAKE_SOURCE_DIR}/inc/public
    ${CMAKE_SOURCE_DIR}/inc/concurrent
    ${CMAKE_SOURCE_DIR}/inc/threadpool

)
find_package(Threads REQUIRED)
add_executable(${PROJECT_NAME} ${SRC_LIST})
target_link_libraries(${PROJECT_NAME} Threads::Threads threadpool)
