#ifndef _IO_SERVICE_POOL_H_
#define _IO_SERVICE_POOL_H_
#include "comm.h"
#include <boost/asio.hpp>


// 封装网络io线程池
class io_service_pool:public Singleton<io_service_pool>
{
	friend Singleton<io_service_pool>;
public:
	using io_service = boost::asio::io_context;
	using work = boost::asio::io_context::work;
	using work_ptr = std::unique_ptr<work>;
	~io_service_pool();
	io_service_pool(const io_service_pool&) = delete;
	io_service_pool& operator=(const io_service_pool&) = delete;
	//  round-robin  io_service
	boost::asio::io_context& get_ioservice();
	void stop();
private:
	io_service_pool(std::size_t size = 2/*std::thread::hardware_concurrency()*/);
	std::vector<io_service> io_service_;
	std::vector<work_ptr> works_;
	std::vector<std::thread> threads_;
	std::size_t                        next_ioservice_;
};


#endif
