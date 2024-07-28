#include "msg_node.h"

recv_node::recv_node (short max_len, short msgid) : msg_node(max_len), msg_id_(msg_id_){}

send_node::send_node (const char *buf, short maxlen, short msgid) : 
msg_node(maxlen + HDR_TOTAL_SIZE), msg_id_(msgid) {
    short msgid_net = boost::asio::detail::socket_ops::host_to_network_short(msgid);
    memcpy(data_, &msgid_net, HDR_ID_SIZE);
    short msglen_net = boost::asio::detail::socket_ops::host_to_network_short(maxlen);
    memcpy(data_ + HDR_ID_SIZE, &msglen_net, HDR_ID_SIZE);
    memcpy(data_ + HDR_TOTAL_SIZE, buf, maxlen);
}