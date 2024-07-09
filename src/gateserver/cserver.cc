#include "cserver.h"
#include "http_conn.h"
#include "ioservice_pool.h"
cserver::cserver(boost::asio::io_context &ioc, unsigned short &port):ioc_(ioc), 
    acceptor_(ioc, tcp::endpoint(tcp::v4(), port)) {

}


void cserver::start() {
    //防止对象被析构，引用计数同步
    auto self = shared_from_this();
    //每次从iocontext池里取出一个创建http连接进行通信，不同所有的连接请求，读写请求都使用一个iocontext
    auto &io_context = ioservice_pool::get_instance()->get_ioservice();
    std::shared_ptr<http_conn> new_conn = std::make_shared<http_conn>(io_context);
    //捕获指针延长生命周期，防止在回调函数调用之前对象已经被析构
    acceptor_.async_accept(new_conn->get_socket(), [self, new_conn](beast::error_code ec){
        try {   
            if (ec) {
                self->start();
                return;
            }
            //socket没有拷贝构造和拷贝赋值，而且下一次连接来的时候socket_会被覆盖所以直接移动过去
            //创建http_connection类管理新连接
            // std::make_shared<http_conn>(std::move(self->socket_))->start();
            // httpconnection(std::move(self->socket_));
            new_conn->start();
            //重新监听连接事件
            self->start();

        } catch (std::exception &e){

        }
    });
}