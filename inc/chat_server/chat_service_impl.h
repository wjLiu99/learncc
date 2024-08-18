#ifndef _CHAT_SERVICE_IMPL_H_
#define _CHAT_SERVICE_IMPL_H_

#include <grpcpp/grpcpp.h>
#include "msg.grpc.pb.h"
#include "msg.pb.h"
#include <mutex>
#include "data.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using message::AddFriendReq;
using message::AddFriendRsp;

using message::AuthFriendReq;
using message::AuthFriendRsp;

using message::ChatService;
using message::TextChatMsgReq;
using message::TextChatMsgRsp;
using message::TextChatData;


class ChatServiceImpl final: public ChatService::Service
{
public:
	ChatServiceImpl();
	Status NotifyAddFriend(ServerContext* context, const AddFriendReq* request,
		AddFriendRsp* reply) override;

	Status NotifyAuthFriend(ServerContext* context, 
		const AuthFriendReq* request, AuthFriendRsp* response) override;

	Status NotifyTextChatMsg(::grpc::ServerContext* context, 
		const TextChatMsgReq* request, TextChatMsgRsp* response) override;

	bool get_base_info(std::string base_key, int uid, std::shared_ptr<user_info>& userinfo);

private:
};
#endif