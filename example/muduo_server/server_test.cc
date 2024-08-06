#include <tcp_server.h>
#include <logger.h>

#include <string>
#include <functional>

class EchoServer
{
public:
    EchoServer(event_loop *loop,
            const inet_address &addr, 
            const std::string &name)
        : server_(loop, addr, name)
        , loop_(loop)
    {
        // 注册回调函数
        server_.set_connection_cb(
            std::bind(&EchoServer::onConnection, this, std::placeholders::_1)
        );

        server_.set_message_cb(
            std::bind(&EchoServer::onMessage, this,
                std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
        );

        // 设置合适的loop线程数量 loopthread
        server_.set_threadnum(3);
    }
    void start()
    {
        server_.start();
    }
private:
    // 连接建立或者断开的回调
    void onConnection(const tcp_connection_ptr &conn)
    {
        if (conn->connected())
        {
            LOG_INFO("Connection UP : %s", conn->remote_address().to_ip_port().c_str());
        }
        else
        {
            LOG_INFO("Connection DOWN : %s", conn->remote_address().to_ip_port().c_str());
        }
    }

    // 可读写事件回调
    void onMessage(const tcp_connection_ptr &conn,
                buffer *buf,
                timestamp time)
    {
        std::string msg = buf->retrieve_all_asstring();
        conn->send(msg);
        // conn->shutdown(); // 写端   EPOLLHUP =》 closeCallback_
    }
    tcp_server server_;
    event_loop *loop_;

};

int main()
{
    event_loop loop;
    inet_address addr(8000);
    EchoServer server(&loop, addr, "EchoServer-01"); // Acceptor non-blocking listenfd  create bind 
    server.start(); // listen  loopthread  listenfd => acceptChannel => mainLoop =>
    loop.loop(); // 启动mainLoop的底层Poller

    return 0;
}