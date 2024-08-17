#ifndef _STATUS_SERVICE_IMPL_H_
#define _STATUS_SERVICE_IMPL_H_
#include <grpcpp/grpcpp.h>
#include "msg.grpc.pb.h"
#include <mutex>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using message::GetChatServerReq;
using message::GetChatServerRsp;
using message::LoginReq;
using message::LoginRsp;
using message::StatusService;

struct chat_server {
	std::string host;
	std::string port;
	std::string name;
	int con_count;
};

class status_service_impl final : public StatusService::Service
{
public:
	status_service_impl();
	// 获取一个chatserver 
	Status GetChatServer(ServerContext* context, const GetChatServerReq* request,
		GetChatServerRsp* reply) override;
	// 登入服务 
	Status Login(ServerContext* context, const LoginReq* request,
		LoginRsp* reply) override;
private:
	// 将token插入redis
	void insert_token(int uid, std::string token);
	// 获取连接数最少的chatserver
	chat_server get_chatserver();
	std::unordered_map<std::string, chat_server> servers_;
	std::mutex server_mtx_;

	// 将token存储在redis中
	// std::unordered_map<int, std::string> tokens_;
	// std::mutex _token_mtx;
};


#endif
