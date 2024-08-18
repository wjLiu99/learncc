#include "msg_node.h"
recv_node::recv_node(short max_len, short msg_id):msg_node(max_len),
_msg_id(msg_id){

}


send_node::send_node(const char* msg, short max_len, short msg_id) : msg_node(max_len + HEAD_TOTAL_LEN)
, _msg_id(msg_id){
	//先发送id, 转为网络字节序
	short msg_id_host = boost::asio::detail::socket_ops::host_to_network_short(msg_id);
	memcpy(data_, &msg_id_host, HEAD_ID_LEN);
	//转为网络字节序
	short max_len_host = boost::asio::detail::socket_ops::host_to_network_short(max_len);
	memcpy(data_ + HEAD_ID_LEN, &max_len_host, HEAD_DATA_LEN);
	memcpy(data_ + HEAD_ID_LEN + HEAD_DATA_LEN, msg, max_len);
}
