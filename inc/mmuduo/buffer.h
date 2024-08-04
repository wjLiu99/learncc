#ifndef _BUFFER_H
#define _BUFFER_H

#include <string>
#include <algorithm>
#include <vector>

class buffer {
public:
    static const size_t prepend = 8;
    static const size_t initial_size = 1024;

    explicit buffer (size_t initsize = initial_size) :
        buf_(prepend + initsize),
        read_(prepend),
        write_(prepend) {}

    size_t readable_bytes () const {
        return write_ - read_;
    }

    size_t writable_bytes () const {
        return buf_.size() - write_;
    }

    size_t prependable_bytes () const {
        return read_;
    }

    //返回可读起始地址
    const char *peek () const {
        return begin() + read_;
    }
    //读数据后调整读写位置
    void retrieve(size_t len) {
        if (len < readable_bytes()) {
            read_ += len;
            return;
        }
        retrieve_all();
    }

    void retrieve_all () {
        read_ = write_ = prepend;
    }

    std::string retrieve_all_asstring () {
        return retrieve_as_string(readable_bytes());
    }

    std::string retrieve_as_string(size_t len) {
        std::string res(peek(), len);
        retrieve(len);
        return res;
    }

    //确保可写空间足够
    void ensure_waitable_bytes (size_t len) {
        if (writable_bytes() < len) {
            make_space(len);
        }
    }

    void append (const char *data, size_t len) {
        ensure_waitable_bytes(len);
        std::copy(data, data + len, begin_write());
        write_ += len;
    }

    const char *begin_write () const {
        return begin() + write_;
    }
    char *begin_write ()  {
        return begin() + write_;
    }

    ssize_t read_fd (int fd, int *saveerrno);
    ssize_t write_fd (int fd, int *saveerrno);



private:
    char *begin () {
        return &*buf_.begin();
    }

    const char *begin () const {
        return &*buf_.begin();
    }

    void make_space (size_t len) {
        if (writable_bytes() + prependable_bytes() < len + prepend) {
            buf_.resize(write_ + len);
            return;
        }

        size_t readable = readable_bytes();
        std::copy(begin() + read_, begin() + write_, begin() + prepend);
        read_ = prepend;
        write_ = prepend + readable;
    }
    std::vector<char> buf_; //缓冲区
    size_t read_;           //读索引
    size_t write_;          //写索引
};
#endif