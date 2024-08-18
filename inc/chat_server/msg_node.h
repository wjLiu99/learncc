#ifndef _MSG_NODE_H
#define _MSG_NODE_H
#include <string>
#include "comm.h"
#include <iostream>
#include <boost/asio.hpp>
using namespace std;
using boost::asio::ip::tcp;
class logic_system;

// 消息节点
class msg_node
{
public:
	msg_node(short max_len) :_total_len(max_len), _cur_len(0) {
		data_ = new char[_total_len + 1]();
		data_[_total_len] = '\0';
	}

	~msg_node() {
		std::cout << "destruct msg_node" << endl;
		delete[] data_;
	}

	void clear() {
		::memset(data_, 0, _total_len);
		_cur_len = 0;
	}

	short _total_len;	// 可接收长度
	short _cur_len;		// 当前长度
	char* data_;		// 数据
};

// 接收节点
class recv_node :public msg_node {
	friend class logic_system;
public:
	recv_node(short max_len, short msg_id);
private:
	short _msg_id;
};

// 发送节点
class send_node:public msg_node {
	friend class logic_system;
public:
	send_node(const char* msg,short max_len, short msg_id);
private:
	short _msg_id;
};


#endif

