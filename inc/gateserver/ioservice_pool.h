#ifndef _IOSERVER_POOL_H_
#define _IOSERVER_POOL_H_

#include "comm.h"

class ioservice_pool : public Singleton<ioservice_pool>
{
    friend Singleton<ioservice_pool>;
public:
    using ioservice = boost::asio::io_context;
    using work = boost::asio::io_context::work;
    using work_ptr = std::unique_ptr<work>;
    ~ioservice_pool();
    ioservice_pool(const ioservice_pool&) = delete;
    ioservice_pool& operator=(const ioservice_pool&) = delete;
    // 使用 round-robin 的方式返回一个 io_service
    boost::asio::io_context& get_ioservice();
    void stop();
private:
    ioservice_pool(std::size_t size = std::thread::hardware_concurrency());
    std::vector<ioservice> ioservices_;
    std::vector<work_ptr> works_;
    std::vector<std::thread> threads_;
    std::size_t  next_ioservice_;
};

#endif