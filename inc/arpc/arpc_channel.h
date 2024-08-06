#ifndef _ARPC_CHANNEL_H_
#define _ARPC_CHANNEL_H_

#include <google/protobuf/service.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <map>
#include <atomic>
#include <mutex>

#define LOG_CHANNEL LOG_MAX
class arpc_channel : public ::google::protobuf::RpcChannel
{
public:

    void CallMethod(const google::protobuf::MethodDescriptor* method,
                          google::protobuf::RpcController* controller, 
                          const google::protobuf::Message* request,
                          google::protobuf::Message* response,
                          google::protobuf:: Closure* done);
private:
    struct out_call {
        ::google::protobuf::Message *response;
        ::google::protobuf::Closure *done;
    };
    std::atomic<int> id_;
    std::mutex mtx_;
    std::map<int, out_call> calls_;



};

#endif