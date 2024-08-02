#ifndef _EPOLL_POLLER_H_
#define _EPOLL_POLLER_H_

#include "poller.h"
#include "timestamp.h"

#include <vector>
#include <sys/epoll.h>

class channel;

class epoll_poller : public poller {
public:
    epoll_poller (event_loop *loop);
    ~epoll_poller ();

    timestamp poll (int tmo, channel_list *active_channels) override;
    void update_channel (channel *ch) override;
    void remove_channel (channel *ch) override;

private:
    static const int init_eventlist_size = 16;
    // 填写就绪的事件
    void fill_active_channels (int event_num, channel_list *active_channels) const;
    // 更新监听事件
    void update (int op, channel *ch);

    typedef std::vector<epoll_event> event_list;
    int epfd_;
    event_list events_; //epollwait返回就绪事件列表

};
#endif