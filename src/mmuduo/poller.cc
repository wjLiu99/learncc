#include "poller.h"
#include "channel.h"

poller::poller (event_loop *loop) : owner_loop_(loop) {

}

bool poller::has_channel (channel *ch) const {
    auto it = channels_.find(ch->fd());
    if (channels_.end() != it && it->second == ch) {
        return true;
    }
    return false;
}