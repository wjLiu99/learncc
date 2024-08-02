#include "epoll_poller.h"
#include "logger.h"
#include "channel.h"


#include <errno.h>
#include <unistd.h>
#include <strings.h>

enum channel_state{
    NEW = -1,   //channel从未添加到poller中，或已经从poller中删除
    ADDED = 1,  //channel已添加到poller中
    DELETED = 2,//channel不监听任何事件
};

epoll_poller::epoll_poller (event_loop *loop)
        :poller(loop), epfd_(::epoll_create1(EPOLL_CLOEXEC)),
        events_(init_eventlist_size) {

    if (epfd_ < 0)
    {
        LOG_FATAL("epoll_create failed. errno = %d \n", errno);
    }
}

epoll_poller::~epoll_poller () {
    ::close(epfd_);
}

timestamp epoll_poller::poll (int tmo, channel_list *active_channels) {
    LOG_INFO("func = %s => fd total count:%lu \n", __FUNCTION__, channels_.size());
    int num = ::epoll_wait(epfd_, &*events_.begin(), static_cast<int>(events_.size()), tmo);
    int save_errno = errno;
    timestamp now(timestamp::now());
    if (num > 0) {
        LOG_INFO("%d events happend.\n", num);
        fill_active_channels(num, active_channels);
        //如果就绪列表满了就扩容
        if (num == events_.size()) {
            events_.resize(events_.size() * 2);
        }
    } else if (num == 0) {
        LOG_INFO("%s timeout.\n", __FUNCTION__);
    } else {
        if (save_errno != EINTR) {
            errno = save_errno;
            LOG_ERROR("epoll poller::poll() failed.");
        }
    }
    return now;
}

void epoll_poller::update_channel (channel *ch) {
    const int idx = ch->index();
    int fd = ch->fd();
    LOG_INFO("func=%s => fd=%d events=%d index=%d \n", __FUNCTION__, ch->fd(), ch->events(), idx);
    //根据channel上设置的事件来修改epoll监听的事件
    if (NEW == idx || DELETED == idx) {
        if (NEW == idx) {
  
            channels_[fd] = ch;
        }
        ch->set_index(ADDED);
        update(EPOLL_CTL_ADD, ch);
        return;
    }
    // 如果channel的状态是已添加，可能是删除或者修改
    if (ch->is_none()) {
        update(EPOLL_CTL_DEL, ch);
        ch->set_index(DELETED);
        return;
    }

    update(EPOLL_CTL_MOD, ch);


}

void epoll_poller::remove_channel (channel *ch) {
    int fd = ch->fd();
    channels_.erase(fd);
    LOG_INFO("func=%s => fd=%d\n", __FUNCTION__, fd);
    int idx = ch->index();
    if (ADDED == idx) {
        update(EPOLL_CTL_DEL, ch);
    }
    //-------------------------
    ch->set_index(NEW);
}

// epoll wait将就绪事件填充到events_中，然后将就绪事件的channel取出填充到eventloop的就绪队列中
void epoll_poller::fill_active_channels (int num, channel_list *active_channels) const{
    for (int i = 0; i < num; ++i) {
        channel *ch = static_cast<channel *>(events_[i].data.ptr);
        ch->set_revents(events_[i].events);
        active_channels->push_back(ch);
    }
}
//通过channel给epoll上添加事件，传给epoll的事件封装了channel指针
void epoll_poller::update (int op, channel *ch) {
    epoll_event ev;
    bzero(&ev, sizeof ev);
    int fd = ch->fd();
    ev.events = ch->events();
    ev.data.ptr = ch;

    if (::epoll_ctl(epfd_, op, fd, &ev) < 0) {
        if (EPOLL_CTL_DEL == op) {
            LOG_ERROR("epoll ctl del failed. errno = %d\n", errno);
            return;
        }
        LOG_FATAL("epoll ctl add/mod failed. errno = %d\n", errno);
    }
}