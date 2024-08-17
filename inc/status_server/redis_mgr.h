#ifndef _REDIS_MGR_H_
#define _REDIS_MGR_H_

#include "comm.h"
#include <hiredis/hiredis.h>
//封装redis连接池
class redis_conn_pool {

public:
    redis_conn_pool (std::size_t size, const char *host, int port, const char *password) : size_(size),
            													host_(host), port_(port), stop_(false) {
		for (size_t i = 0; i < size_; ++i) {
			// 连接redis数据库
			auto* context = redisConnect(host, port);
			if (context == nullptr || context->err != 0) {
				if (context != nullptr) {
					redisFree(context);
				}
				continue;
			}

			auto reply = (redisReply*)redisCommand(context, "AUTH %s", password);
			if (reply->type == REDIS_REPLY_ERROR) {
				std::cout << "认证失败" << std::endl;
				//释放redisCommand执行后返回的redisReply所占用的内存
				redisFree(context);
				freeReplyObject(reply);
				continue;
			}

			//执行成功 释放redisCommand执行后返回的redisReply所占用的内存
			freeReplyObject(reply);
			std::cout << "认证成功" << std::endl;
			conn_.push(context);
	}  
    }

    ~redis_conn_pool () {
		std::lock_guard<std::mutex> lock(mtx_);
		// close();
		while (!conn_.empty()) {
			auto context = conn_.front();
			redisFree(context);
			conn_.pop();
		}
	}

    redisContext* get_conn() {
		std::unique_lock<std::mutex> lock(mtx_);
		cond_.wait(lock, [this] { 
			if (stop_) {
				return true;
			}
			return !conn_.empty(); 
			});
		//如果停止则直接返回空指针
		if (stop_) {
			return  nullptr;
		}
		auto context = conn_.front();
		conn_.pop();
		return context;
	}
    //归还一个连接
    void return_conn(redisContext* context) {
		std::lock_guard<std::mutex> lock(mtx_);
		conn_.push(context);
		cond_.notify_one();
	}

	void close() {
		stop_ = true;
		cond_.notify_all();
	}



private:
    
    std::size_t size_;
    const char *host_;
    int port_;
    std::atomic<bool> stop_;
    std::queue<redisContext *> conn_;
    std::mutex mtx_;
    std::condition_variable cond_;
};

// 封装redis基本操作 
class redis_mgr: public Singleton<redis_mgr>
{
	friend class Singleton<redis_mgr>;
public:
	~redis_mgr();
	bool get(const std::string &key, std::string& value);
	bool set(const std::string &key, const std::string &value);
	bool lpush(const std::string &key, const std::string &value);
	bool lpop(const std::string &key, std::string& value);
	bool rpush(const std::string& key, const std::string& value);
	bool rpop(const std::string& key, std::string& value);
	bool hset(const std::string &key, const std::string  &hkey, const std::string &value);
	bool hset(const char* key, const char* hkey, const char* hvalue, size_t hvaluelen);
	std::string hget(const std::string &key, const std::string &hkey);
	bool del(const std::string &key);
	bool exist_key(const std::string &key);
	void close();
private:
	redis_mgr();
	std::unique_ptr<redis_conn_pool>  con_pool_;
};


#endif