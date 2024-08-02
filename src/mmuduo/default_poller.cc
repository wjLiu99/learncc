#include "poller.h"
#include "epoll_poller.h"

#include <stdlib.h>

poller* poller::default_poller(event_loop *loop)
{
    if (::getenv("MUDUO_USE_POLL"))
    {
        return nullptr; // 生成poll的实例
    }
    else
    {
        return new epoll_poller(loop); // 生成epoll的实例
    }
}