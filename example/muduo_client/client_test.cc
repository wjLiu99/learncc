#include "tcp_client.h"

#include "logger.h"
// #include "muduo/base/Thread.h"
#include "event_loop.h"
#include "inet_addr.h"

#include <utility>

#include <stdio.h>
#include <unistd.h>


int numThreads = 0;
class EchoClient;
std::vector<std::unique_ptr<EchoClient>> clients;
int current = 0;
using namespace std::placeholders;
class EchoClient : noncopyable
{
 public:
  EchoClient(event_loop* loop, const inet_address& listenAddr, const std::string& id)
    : loop_(loop),
      client_(loop, listenAddr, "EchoClient"+id)
  {
    client_.set_connection_cb(
        std::bind(&EchoClient::onConnection, this, _1));
    client_.set_message_cb(
        std::bind(&EchoClient::onMessage, this, _1, _2, _3));
    //client_.enableRetry();
  }

  void connect()
  {
    client_.connect();
  }
  // void stop();

 private:
  void onConnection(const tcp_connection_ptr& conn)
  {
//     LOG_TRACE << conn->localAddress().toIpPort() << " -> "
//         << conn->peerAddress().toIpPort() << " is "
//         << (conn->connected() ? "UP" : "DOWN");

    if (conn->connected())
    {
      ++current;
      if ((size_t)(current) < clients.size())
      {
        clients[current]->connect();
      }
    //   LOG_INFO << "*** connected " << current;
    }
    conn->send("world\n");
  }

  void onMessage(const tcp_connection_ptr& conn, buffer* buf, timestamp time)
  {
    std::string msg(buf->retrieve_all_asstring());
    // LOG_TRACE << conn->name() << " recv " << msg.size() << " bytes at " << time.toString();
    if (msg == "quit\n")
    {
      conn->send("bye\n");
      conn->shutdown();
    }
    else if (msg == "shutdown\n")
    {
      loop_->quit();
    }
    else
    {
      conn->send(msg);
    }
  }

  event_loop* loop_;
  tcp_client client_;
};

int main(int argc, char* argv[])
{
//   LOG_INFO << "pid = " << getpid() << ", tid = " << CurrentThread::tid();
  if (argc > 1)
  {
    event_loop loop;
    inet_address serverAddr(8000, argv[1]);

    int n = 5;
    if (argc > 2)
    {
      n = atoi(argv[2]);
    }

    clients.reserve(n);
    for (int i = 0; i < n; ++i)
    {
      char buf[32];
      snprintf(buf, sizeof buf, "%d", i+1);
      clients.emplace_back(new EchoClient(&loop, serverAddr, buf));
    }

    clients[current]->connect();
    loop.loop();
  }
  else
  {
    printf("Usage: %s host_ip [current#]\n", argv[0]);
  }
}
