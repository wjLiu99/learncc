#ifndef _CALLBACKS_H_
#define _CALLBACKS_H_


#include <memory>
#include <functional>

class buffer;
class tcp_connection;
class timestamp;

typedef std::shared_ptr<tcp_connection> tcp_connection_ptr;
typedef std::function<void (const tcp_connection_ptr&)> connection_cb;
typedef std::function<void (const tcp_connection_ptr &)> close_cb;
typedef std::function<void (const tcp_connection_ptr&)> write_complete_cb;
typedef std::function<void (const tcp_connection_ptr&,
                                        buffer*,
                                        timestamp)> message_cb;

typedef std::function<void (const tcp_connection_ptr&, size_t)> high_watermark_cb;
#endif