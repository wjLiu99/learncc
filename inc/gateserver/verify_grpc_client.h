#ifndef _VERIFY_GRPC_CLIENT_H_
#define _VERIFY_GRPC_CLIENT_H_

#include <grpcpp/grpcpp.h>
#include "msg.grpc.pb.h"
#include "comm.h"
#include "singleton.h"

using grpc::Channel;
using grpc::Status;
using grpc::ClientContext;

using message::get_verify_req;
using message::get_verify_rsp;
using message::verify_service;

class verify_grpc_client : public Singleton<verify_grpc_client> {
    friend class Singleton<verify_grpc_client>;
public:
    get_verify_rsp get_verify_code (std::string email) {
        ClientContext context;
        get_verify_rsp rsp;
        get_verify_req req;
        req.set_email(email);
        Status status = stub_->get_verify_code(&context, req, &rsp);
        if (status.ok()) {
            return rsp;
        } else {
            rsp.set_error(ERR_RPC);
            return rsp;
        }   

    }
private:
    verify_grpc_client () {
        std::shared_ptr<Channel> channel = grpc::CreateChannel("0.0.0.0:50051",
        grpc::InsecureChannelCredentials());
        stub_ = verify_service::NewStub(channel);
    }
    
    std::unique_ptr<verify_service::Stub> stub_;
};
#endif