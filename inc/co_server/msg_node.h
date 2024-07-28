#include <string>
#include <iostream>
#include <boost/asio.hpp>

#include "comm.h"

class msg_node {
public:
    msg_node (short max_len) : total_len_(max_len), cur_len_(0){
        data_ = new char[total_len_ + 1]();
        data_[total_len_] = '\0';
    }
    ~msg_node () {
        std::cout << "destruct msg node" << std::endl;
        delete []data_;
    }

    void clear () {
        memset(data_, 0, total_len_);
        cur_len_ = 0;
    }

protected:

    short cur_len_;
    short total_len_;
    char *data_;
};


class  recv_node : public msg_node
{
private:
    short msg_id_;
public:
    recv_node (short max_len, short msg_id);
    ~ recv_node ();
};

class send_node :public msg_node{
private:
    short msg_id_;
public:
    send_node (const char *buf, short max_len, short msgid);
    ~ send_node();
};