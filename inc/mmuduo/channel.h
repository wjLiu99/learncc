#ifndef _CHANNEL_H_
#define _CHANNEL_H_

#include "noncopyable.h"
#include "timestamp.h"
#include <memory>
#include <functional>

class event_loop;
class channel : noncopyable {
public :
    typedef std::function<void()> event_cb;
    typedef std::function<void(timestamp)> read_cb;
    channel (event_loop *loop, int fd);
    ~channel ();
    //poller返回后处理事件
    void handle_event (timestamp receive_time);

    //设置回调函数
    void set_read_cb (read_cb cb) { read_cb_ = std::move(cb); }
    void set_write_cb (event_cb cb) { write_cb_ = std::move(cb); }
    void set_close_cb (event_cb cb) { close_cb_ = std::move(cb); }
    void set_error_cb (event_cb cb) { error_cb_ = std::move(cb); }

    void tie (const std::shared_ptr<void> &);

    int fd () const { return fd_; }
    int events () const {return events_; }
    //poller中调用设置满足条件的事件
    void set_revents (int revnets) { revents_ = revnets; }

    void enable_reading () { events_ |= kread_event; update(); }
    void disable_reading () { events_ &= ~kread_event; update(); }
    void enable_writing () { events_ |= kwrite_event; update(); }
    void disable_writing () { events_ &= ~kwrite_event; update(); }
    void disable_all () { events_ = knone_event; update(); }


    //当前fd监听的事件
    bool is_none () { return events_ == knone_event; }
    bool is_reading ()  {return events_ == kread_event; }
    bool is_writing () {return events_ == kwrite_event; }


    int index () { return idx_; }
    void set_index (int idx) { idx_ = idx; }

    event_loop *owner_loop () { return loop_; }
    void remove ();

private :

    void update ();
    void handle_event_with_guard (timestamp receive_time);
    //监听以及满足的事件类型
    static const int knone_event;
    static const int kread_event;
    static const int kwrite_event;

    event_loop *loop_;
    const int fd_;
    int events_;
    int revents_;
    int idx_;

    std::weak_ptr<void> tie_;
    bool tied_;

    read_cb read_cb_;
    event_cb write_cb_;
    event_cb close_cb_;
    event_cb error_cb_;


};
#endif