project(learn)
file(GLOB_RECURSE SRC_LIST "*.cc")

include_directories(
    ${CMAKE_SOURCE_DIR}/inc/learncpp
    ${CMAKE_SOURCE_DIR}/inc/public
)

add_executable(${PROJECT_NAME} ${SRC_LIST})
target_link_libraries(${PROJECT_NAME} dl)
