#include "status_grpc_client.h"

GetChatServerRsp status_grpc_client::GetChatServer(int uid)
{
	ClientContext context;
	GetChatServerRsp reply;
	GetChatServerReq request;
	request.set_uid(uid);
	auto stub = pool_->get_conn();
	Status status = stub->GetChatServer(&context, request, &reply);
	Defer defer([&stub, this]() {
		pool_->return_conn(std::move(stub));
		});
	if (status.ok()) {	
		return reply;
	}
	else {
		reply.set_error(ERR_RPC);
		return reply;
	}
}

LoginRsp status_grpc_client::Login(int uid, std::string token)
{
	ClientContext context;
	LoginRsp reply;
	LoginReq request;
	request.set_uid(uid);
	request.set_token(token);

	auto stub = pool_->get_conn();
	Status status = stub->Login(&context, request, &reply);
	Defer defer([&stub, this]() {
		pool_->return_conn(std::move(stub));
		});
	if (status.ok()) {
		return reply;
	}
	else {
		reply.set_error(ERR_RPC);
		return reply;
	}
}


status_grpc_client::status_grpc_client()
{
	auto& gCfgMgr = conf_mgr::get_instance();
	std::string host = gCfgMgr["status_server"]["host"];
	std::string port = gCfgMgr["status_server"]["port"];
	pool_.reset(new status_grpc_conpool(5, host, port));
}