#ifndef _CHAT_GRPC_CLIENT_H_
#define _CHAT_GRPC_CLIENT_H_


#include "singleton.h"
#include "conf_mgr.h"
#include <grpcpp/grpcpp.h> 
#include "msg.grpc.pb.h"
#include "msg.pb.h"
#include <queue>
#include "comm.h"
#include "data.h"
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>

using grpc::Channel;
using grpc::Status;
using grpc::ClientContext;

using message::AddFriendReq;
using message::AddFriendRsp;

using message::AuthFriendReq;
using message::AuthFriendRsp;

using message::GetChatServerRsp;
using message::LoginRsp;
using message::LoginReq;
using message::ChatService;

using message::TextChatMsgReq;
using message::TextChatMsgRsp;
using message::TextChatData;


class chat_grpc_conpool {
public:
	chat_grpc_conpool(size_t pool_size, std::string host, std::string port)
		: pool_size_(pool_size), host_(host), port_(port), b_stop_(false) {
		for (size_t i = 0; i < pool_size_; ++i) {

			std::shared_ptr<Channel> channel = grpc::CreateChannel(host + ":" + port,
				grpc::InsecureChannelCredentials());
            // 右值可以直接push，移动语义
			connections_.push(ChatService::NewStub(channel));
		}
	}

	~chat_grpc_conpool() {
		std::lock_guard<std::mutex> lock(mutex_);
		close();
		while (!connections_.empty()) {
			connections_.pop();
		}
	}

	std::unique_ptr<ChatService::Stub> get_conn() {
		std::unique_lock<std::mutex> lock(mutex_);
		cond_.wait(lock, [this] {
			if (b_stop_) {
				return true;
			}
			return !connections_.empty();
			});
		//如果停止则直接返回空指针
		if (b_stop_) {
			return  nullptr;
		}
		auto context = std::move(connections_.front());
		connections_.pop();
		return context;
	}

	void return_conn(std::unique_ptr<ChatService::Stub> context) {
		std::lock_guard<std::mutex> lock(mutex_);
		if (b_stop_) {
			return;
		}
		connections_.push(std::move(context));
		cond_.notify_one();
	}

	void close() {
		b_stop_ = true;
		cond_.notify_all();
	}

private:
	std::atomic<bool> b_stop_;
	size_t pool_size_;
	std::string host_;
	std::string port_;
	std::queue<std::unique_ptr<ChatService::Stub> > connections_;
	std::mutex mutex_;
	std::condition_variable cond_;
};

// 编译阶段模版不会做过多检查，运行阶段ChatGrpcClient已经生成了所以可以继承
// rpc客户端
class chat_grpc_client :public Singleton<chat_grpc_client>
{
	friend class Singleton<chat_grpc_client>;
public:
	~chat_grpc_client() {

	}

	AddFriendRsp NotifyAddFriend(std::string server_ip, const AddFriendReq& req);
	AuthFriendRsp NotifyAuthFriend(std::string server_ip, const AuthFriendReq& req);
	TextChatMsgRsp NotifyTextChatMsg(std::string server_ip, const TextChatMsgReq& req, const Json::Value& rtvalue);

private:
	chat_grpc_client();
    
	bool get_base_info(std::string base_key, int uid, std::shared_ptr<user_info>& userinfo);
	// 对端名 ： 连接池
	std::unordered_map<std::string, std::unique_ptr<chat_grpc_conpool>> pools_;	
};

#endif