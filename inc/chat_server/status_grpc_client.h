#ifndef _STATUS_GRPC_CLIENT_H_
#define _STATUS_GRPC_CLIENT_H_
#include "comm.h"
#include "singleton.h"
#include "conf_mgr.h"
#include "msg.grpc.pb.h"
#include "msg.pb.h"
#include <grpcpp/grpcpp.h>
#include <queue>
#include <condition_variable>
#include <atomic>
using grpc::Channel;
using grpc::Status;
using grpc::ClientContext;

using message::GetChatServerReq;
using message::GetChatServerRsp;
using message::LoginRsp;
using message::LoginReq;
using message::StatusService;

class StatusConPool {
public:
	StatusConPool(size_t pool_size, std::string host, std::string port)
		: b_stop_(false), pool_size_(pool_size), host_(host), port_(port) {
		for (size_t i = 0; i < pool_size_; ++i) {

			std::shared_ptr<Channel> channel = grpc::CreateChannel(host + ":" + port,
				grpc::InsecureChannelCredentials());

			connections_.push(StatusService::NewStub(channel));
		}
	}

	~StatusConPool() {
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

class StatusGrpcClient :public Singleton<StatusGrpcClient>
{
	friend class Singleton<StatusGrpcClient>;
public:
	~StatusGrpcClient() {

	}
	GetChatServerRsp GetChatServer(int uid);
	LoginRsp Login(int uid, std::string token);
private:
	StatusGrpcClient();
	std::unique_ptr<StatusConPool> pool_;
	
};


#endif


