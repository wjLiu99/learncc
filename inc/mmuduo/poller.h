#ifndef _POLLER_H_
#define _POLLER_H_

#include "noncopyable.h"
#include "timestamp.h"

#include <vector>
#include <unordered_map>

class channel;
class event_loop;

class poller : noncopyable {
public:
    typedef std::vector<channel *> channel_list;

    poller (event_loop *loop);
    virtual ~poller () = default;
    // 给不同的io多路复用留下的接口
    virtual timestamp poll (int tmo, channel_list *active_channel) = 0;
    virtual void update_channel (channel *ch) = 0;
    virtual void remove_channel (channel *ch) = 0;

    bool has_channel (channel *ch) const;
    
    static poller *default_poller (event_loop *loop);

protected:
    typedef std::unordered_map<int, channel *> channel_map;
    channel_map channels_;


private:
    event_loop *owner_loop_;

};



#endif  