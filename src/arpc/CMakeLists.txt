project(arpc)
file(GLOB_RECURSE SRC_LIST "*.cc")


include_directories(
    ${CMAKE_SOURCE_DIR}/inc/arpc
    ${CMAKE_SOURCE_DIR}/inc/mmuduo
    ${CMAKE_SOURCE_DIR}/inc/public
)


add_library(${PROJECT_NAME}  ${SRC_LIST})
# add_executable(${PROJECT_NAME} ${SRC_LIST})
# target_link_libraries(${PROJECT_NAME} mmuduo muduo_net muduo_base pthread protobuf)
target_link_libraries(${PROJECT_NAME} mmuduo  pthread protobuf)

