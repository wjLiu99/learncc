#include "ioservice_pool.h"
/* 
    没有拷贝构造就要用初始化列表进行初始化，vector中传入size大小就会调用对象的构造函数创建对象
*/

ioservice_pool::ioservice_pool(std::size_t size):ioservices_(size),
works_(size), next_ioservice_(0){
    for (std::size_t i = 0; i < size; ++i) {
        works_[i] = std::unique_ptr<work>(new work(ioservices_[i]));
    }
    //遍历多个ioservice，创建多个线程，每个线程内部启动ioservice
    for (std::size_t i = 0; i < ioservices_.size(); ++i) {
        threads_.emplace_back([this, i]() {
            ioservices_[i].run();
            });
    }
}

ioservice_pool::~ioservice_pool() {
    //RAII 自己创建资源析构中释放
    stop();
    std::cout << "ioservice pool destruct" << std::endl;
}

boost::asio::io_context& ioservice_pool::get_ioservice() {
    auto& service = ioservices_[next_ioservice_++];
    if (next_ioservice_ == ioservices_.size()) {
        next_ioservice_ = 0;
    }
    return service;
}

void ioservice_pool::stop(){
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