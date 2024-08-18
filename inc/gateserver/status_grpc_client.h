#ifndef _STATUS_CLIENT_H_
#define _STATUS_CLIENT_H_
#include "comm.h"
#include "singleton.h"
#include "conf_mgr.h"
#include <grpcpp/grpcpp.h> 
#include "msg.grpc.pb.h"
#include "msg.pb.h"

using grpc::Channel;
using grpc::Status;
using grpc::ClientContext;

using message::GetChatServerReq;
using message::GetChatServerRsp;
using message::LoginRsp;
using message::LoginReq;
using message::StatusService;

class status_grpc_conpool {
public:
	status_grpc_conpool(size_t pool_size, std::string host, std::string port)
		: pool_size_(pool_size), host_(host), port_(port), b_stop_(false) {
		for (size_t i = 0; i < pool_size_; ++i) {

			std::shared_ptr<Channel> channel = grpc::CreateChannel(host + ":" + port,
				grpc::InsecureChannelCredentials());

			connections_.push(StatusService::NewStub(channel));
		}
	}

	~status_grpc_conpool() {
		std::lock_guard<std::mutex> lock(mutex_);
		close();
		while (!connections_.empty()) {
			connections_.pop();
		}
	}

	std::unique_ptr<StatusService::Stub> get_conn() {
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

	void return_conn(std::unique_ptr<StatusService::Stub> context) {
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
	std::queue<std::unique_ptr<StatusService::Stub>> connections_;
	std::mutex mutex_;
	std::condition_variable cond_;
};

class status_grpc_client :public Singleton<status_grpc_client>
{
	friend class Singleton<status_grpc_client>;
public:
	~status_grpc_client() {

	}
	// 获取chatserver地址
	GetChatServerRsp GetChatServer(int uid);
	// chatserver登入时查询token是否正确，将token保存到redis中该函数可以不使用，chatserver直接去查redis数据库
	LoginRsp Login(int uid, std::string token);
private:
	status_grpc_client();
	std::unique_ptr<status_grpc_conpool> pool_;
	
};
#endif