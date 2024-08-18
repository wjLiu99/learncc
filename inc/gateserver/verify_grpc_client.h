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



//rpc连接池
class rpc_pool {

public:
    rpc_pool(std::size_t size, std::string host, std::string port): pool_size_(size), 
            host_(host), port_(port), stop_(false) {
        for (size_t i = 0; i < pool_size_; ++i) {
            std::shared_ptr<Channel> channel = grpc::CreateChannel(host+":"+ port,
            grpc::InsecureChannelCredentials());

            //返回的是一个unique_ptr类型，为什么可以push，返回的是一个右值，可以移动构造
            conn_.push(verify_service::NewStub(channel));
            //这样不行，m是一个左值，uniqueptr没有拷贝构造函数
            // auto m = verify_service::NewStub(channel);
            // conn_.push(m);
        }      
    }

    ~rpc_pool () {
        std::lock_guard<std::mutex> lk(mtx_);
        close();
        while (!conn_.empty()) {
            conn_.pop();
        }
    }
    //取出一个连接
    std::unique_ptr<verify_service::Stub> get_conn () {
        std::unique_lock<std::mutex> lk(mtx_);
        cond_.wait(lk, [this]{
            if (stop_) {
                return true;
            }
            return !conn_.empty();
        });

        if (stop_) {
            return nullptr;
        }
        //unique只能移动
        auto conn = std::move(conn_.front());
        conn_.pop();
        return conn;
    }

    void return_conn (std::unique_ptr<verify_service::Stub> conn) {
        std::lock_guard<std::mutex> lk(mtx_);
        if (stop_) {
            return;
        }
        //归还一个链接，通知线程
        conn_.push(std::move(conn));
        cond_.notify_one();
    }
    void close () {
        stop_ = true;
        cond_.notify_all();
    }

private:
    
    std::size_t pool_size_;
    std::string host_;
    std::string port_;
    std::atomic<bool> stop_;
    std::queue<std::unique_ptr<verify_service::Stub>> conn_;    //可以使用链表实现队列，头尾分开处理
    std::mutex mtx_;
    std::condition_variable cond_;
};



class verify_grpc_client : public Singleton<verify_grpc_client> {
    friend class Singleton<verify_grpc_client>;
public:
    get_verify_rsp get_verify_code (std::string email);

private:
    verify_grpc_client ();
    std::unique_ptr<rpc_pool> pool_;
    // std::unique_ptr<verify_service::Stub> stub_;  //多线程访问stub会有隐患
};
#endif