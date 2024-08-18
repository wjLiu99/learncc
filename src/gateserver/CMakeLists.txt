project(gateserver)


set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

file(GLOB_RECURSE SRC_LIST "*.cc")

include_directories(

    ${CMAKE_SOURCE_DIR}/inc/gateserver
)

find_package(Threads REQUIRED)
set(protobuf_MODULE_COMPATIBLE TRUE)
find_package(Protobuf CONFIG REQUIRED)
message(STATUS "Using protobuf ${Protobuf_VERSION}")
set(_PROTOBUF_LIBPROTOBUF protobuf::libprotobuf)
set(_REFLECTION gRPC::grpc++_reflection)

find_package(gRPC CONFIG REQUIRED)
message(STATUS "Using gRPC ${gRPC_VERSION}")
set(_GRPC_GRPCPP gRPC::grpc++)


add_executable(${PROJECT_NAME} ${SRC_LIST})

target_link_libraries(  ${PROJECT_NAME} 
                        ${_REFLECTION}
                        ${_GRPC_GRPCPP}
                        ${_PROTOBUF_LIBPROTOBUF} 
                        boost_filesystem     
                        z 
                        jsoncpp 
                        hiredis
                        mysqlcppconn )
