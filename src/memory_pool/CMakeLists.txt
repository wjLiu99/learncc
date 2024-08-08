project(memory_pool)
file(GLOB_RECURSE SRC_LIST "*.cc")


include_directories(

    ${CMAKE_SOURCE_DIR}/inc/memory_pool
    ${CMAKE_SOURCE_DIR}/inc/public
)


# add_library(${PROJECT_NAME}  ${SRC_LIST})
add_executable(${PROJECT_NAME} ${SRC_LIST})
target_link_libraries(${PROJECT_NAME} pthread)
