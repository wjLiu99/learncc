project(muduo_server)
file(GLOB_RECURSE SRC_LIST "*.cc")


include_directories(

    ${CMAKE_SOURCE_DIR}/inc/mmuduo
)

find_package(Threads REQUIRED)

add_executable(${PROJECT_NAME} ${SRC_LIST})
target_link_libraries(${PROJECT_NAME} Threads::Threads mmuduo)
