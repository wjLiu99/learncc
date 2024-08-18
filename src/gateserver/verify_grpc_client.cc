#include "verify_grpc_client.h"

verify_grpc_client::verify_grpc_client () {
        auto& gCfgMgr = conf_mgr::get_instance();
        std::string host = gCfgMgr["verify_server"]["host"];
        std::string port = gCfgMgr["verify_server"]["port"];
        pool_.reset(new rpc_pool(5, host, port));
}

get_verify_rsp verify_grpc_client::get_verify_code (std::string email) {
        ClientContext context;
        get_verify_rsp rsp;
        get_verify_req req;
        req.set_email(email);
        //从连接池取出连接
        auto stub = pool_->get_conn();
        Status status = stub->get_verify_code(&context, req, &rsp);
        //成功失败都要归还连接
        if (status.ok()) {
            pool_->return_conn(std::move(stub));
            return rsp;
        } else {
            pool_->return_conn(std::move(stub));
            rsp.set_error(ERR_RPC);
            return rsp;
        }   

}