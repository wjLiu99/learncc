#include "io_service_pool.h"
#include <iostream>
using namespace std;
io_service_pool::io_service_pool(std::size_t size) : io_service_(size),
works_(size), next_ioservice_(0){
	for (std::size_t i = 0; i < size; ++i) {
		works_[i] = std::unique_ptr<work>(new work(io_service_[i]));
	}

	//遍历多个ioservice，创建多个线程，每个线程内部启动ioservice
	for (std::size_t i = 0; i < io_service_.size(); ++i) {
		threads_.emplace_back([this, i]() {
			io_service_[i].run();
			});
	}
}

io_service_pool::~io_service_pool() {
	stop();
	std::cout << "io_service_pool destruct" << endl;
}

boost::asio::io_context& io_service_pool::get_ioservice() {
	auto& service = io_service_[next_ioservice_++];
	if (next_ioservice_ == io_service_.size()) {
		next_ioservice_ = 0;
	}
	return service;
}

void io_service_pool::stop(){
	//因为仅仅执行work.reset并不能让iocontext从run的状态中退出
	//当iocontext已经绑定了读或写的监听事件后，还需要手动stop该服务。
	for (auto& work : works_) {
		//把服务先停止
		work->get_io_context().stop();
		work.reset();
	}

	for (auto& t : threads_) {
		t.join();
	}
}
