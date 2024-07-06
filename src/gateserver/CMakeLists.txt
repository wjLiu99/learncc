project(gateserver)
file(GLOB_RECURSE SRC_LIST "*.cc")

include_directories(

    ${PROJECT_SOURCE_DIR}/inc/gateserver
)

find_package(Threads REQUIRED)
add_executable(${PROJECT_NAME} ${SRC_LIST})
target_link_libraries(${PROJECT_NAME} Threads::Threads jsoncpp)
