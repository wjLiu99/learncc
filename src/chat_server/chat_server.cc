
#include "logic_system.h"
#include <csignal>
#include <thread>
#include <mutex>
#include "io_service_pool.h"
#include "cserver.h"
#include "conf_mgr.h"
#include "redis_mgr.h"
#include "chat_service_impl.h"
using namespace std;
bool bstop = false;
std::condition_variable cond_quit;
std::mutex mutex_quit;

/*
 token 和 验证码 uid和server的对应关系 保存在redis中
 uid和session的绑定是在内存中管理的，client1和client2通信需要通过server1查询redis
 找到对方uid所在server的ip地址和端口，发送rpc请求给sever2，server2通过uid和session的映射关系将数据发送给user2

*/
int main()
{
	auto& cfg = conf_mgr::get_instance();
	auto server_name = cfg["self_server"]["name"];
	try {
		auto pool = io_service_pool::get_instance();
		//将登录数设置为0
		redis_mgr::get_instance()->hset(LOGIN_COUNT, server_name, "0");

		//定义一个GrpcServer
		std::string server_address(cfg["self_server"]["host"] + ":" + cfg["self_server"]["rpc_port"]);
		ChatServiceImpl service;
		grpc::ServerBuilder builder;
		// 监听端口和添加服务
		builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
		builder.RegisterService(&service);
		// 构建并启动gRPC服务器
		std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
		std::cout << "RPC Server listening on " << server_address << std::endl;

		//单独启动一个线程处理grpc服务
		std::thread  grpc_server_thread([&server]() {
				server->Wait();
			});

		boost::asio::io_context  io_context;
		boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
		signals.async_wait([&io_context, pool, &server](auto, auto) {
			io_context.stop();
			pool->stop();
			server->Shutdown();
			});
		auto port_str = cfg["self_server"]["port"];
		cserver s(io_context, atoi(port_str.c_str()));
		io_context.run();
		redis_mgr::get_instance()->hdel(LOGIN_COUNT, server_name);
		redis_mgr::get_instance()->close();
		grpc_server_thread.join();
	}
	catch (std::exception& e) {
		std::cerr << "Exception: " << e.what() << endl;
		redis_mgr::get_instance()->hdel(LOGIN_COUNT, server_name);
		redis_mgr::get_instance()->close();
	}


}

