#include "arpc_channel.h"
#include "arpc_header.pb.h"
#include "arpc.h"
#include "arpc_controller.h"
#include <unistd.h>
#include <arpa/inet.h>
#include "synclog.h"

using namespace google::protobuf;
using namespace std;
void arpc_channel::CallMethod(const google::protobuf::MethodDescriptor* method,
                                google::protobuf::RpcController* controller, 
                                const google::protobuf::Message* request,
                                google::protobuf::Message* response,
                                google::protobuf:: Closure* done) {

    const ServiceDescriptor *desc = method->service();
    string service_name = desc->name();
    string method_name = method->name();

    
    string args_str;
    if (!request->SerializeToString(&args_str)) {
        if (!controller) return;
        controller->SetFailed("serialize request failed.");
        return;
    }
    uint32_t args_size = args_str.size();

    int id = id_.fetch_add(1);
    // 填充请求头
    arpc_header arpc_hdr;
    arpc_hdr.set_type(REQUEST);
    arpc_hdr.set_id(id);

    arpc_hdr.set_service_name(service_name);
    arpc_hdr.set_method_name(method_name);
    arpc_hdr.set_args_size(args_size);

    out_call out = {response, done};
    {
        lock_guard<mutex> lk(mtx_);
        calls_[id] = out;
    }
    string arpc_hdr_str;
    if (!arpc_hdr.SerializeToString(&arpc_hdr_str)) {
        if (!controller) return;
        controller->SetFailed("arpc header erialize failed.");
        return;
    }

    uint32_t arpc_hdr_size = arpc_hdr_str.size();

    string send_str;
    send_str.insert(0, string((char *)&arpc_hdr_size, 4));
    send_str += arpc_hdr_str;
    send_str += args_str;

    LOG_INFO(LOG_CHANNEL, "============================================\n", 
    "header_size : {}\n", arpc_hdr_size, 
    "arpc_header_str : {}\n", arpc_hdr_str,
    "service_name : {}\n", service_name,
    "method_name : {}\n", method_name,
    "args_str : {}\n", args_str, "============================================\n");


    int s = ::socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == s) {
        char tmp[64] = {0};
        sprintf(tmp, "create socket failed, errno : %d", errno);
        LOG_ERROR(LOG_CHANNEL, tmp);
        if (!controller) return;
        controller->SetFailed(tmp);
        return;
    }

    sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(8000);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");

    int ret = ::connect(s, (sockaddr *)&server, sizeof server);
    if (ret < 0) {
        ::close(s);
        
        char tmp[64] = {0};
        sprintf(tmp, "connect failed, errno : %d", errno);
        LOG_ERROR(LOG_CHANNEL, tmp);
        if (!controller) return;
        controller->SetFailed(tmp);

        return;
    }
   

    ret = ::send(s, send_str.c_str(), send_str.size(), 0);
    if (ret < 0) {
        ::close(s);
        
        char tmp[64] = {0};
        sprintf(tmp, "send failed, errno : %d", errno);
        LOG_ERROR(LOG_CHANNEL, tmp);
        if (!controller) return;
        controller->SetFailed(tmp);
        return;
    }

    thread t([&, s] () {
        char recv_buf[1024] = {0};
        int n = ::recv(s, recv_buf, sizeof recv_buf, 0);
        if (n < 0) {
            ::close(s);
            
            char tmp[64] = {0};
            sprintf(tmp, "recv failed, errno : %d", errno);
            LOG_ERROR(LOG_CHANNEL, tmp);
            if (!controller) return;
            controller->SetFailed(tmp);
            return;
        }
        ::close(s);

        string recv_str(recv_buf, n);
        uint32_t recv_hdr_size = 0;
        recv_str.copy((char *)&recv_hdr_size, 4, 0);

        string recv_arpc_hdr_str = recv_str.substr(4, recv_hdr_size);
        arpc_header recv_arpc_hdr;
        if (!recv_arpc_hdr.ParseFromString(recv_arpc_hdr_str)) {
            char tmp[64] = {0};
            sprintf(tmp, "parse recv arpc header failed");
            LOG_ERROR(LOG_CHANNEL, tmp);
            controller->SetFailed(tmp);
            return;
        }
        if (recv_arpc_hdr.type() == RESPONSE) {
            int recv_id = recv_arpc_hdr.id();
            out_call o = {nullptr, nullptr};
            {
                lock_guard<mutex> lk(mtx_);
                auto it = calls_.find(recv_id);
                if (calls_.end() == it) {
                    char tmp[64] = {0};
                    sprintf(tmp, "rpc call not find");
                    LOG_ERROR(LOG_CHANNEL, tmp);
                    controller->SetFailed(tmp);
                    return;

                }
                o = it->second;
                calls_.erase(it);
            }
            if (o.response) {
                string response_str = recv_str.substr(4 + recv_hdr_size, recv_arpc_hdr.args_size());
                if (!o.response->ParseFromString(response_str)) {
                    char tmp[64] = {0};
                    sprintf(tmp, "parse response failed");
                    LOG_ERROR(LOG_CHANNEL, tmp);
                    controller->SetFailed(tmp);
                    return;
                }

                if (o.done) {
                    o.done->Run();
                }
                delete o.response;

            }

            
        }
       
    });
    t.detach();

}