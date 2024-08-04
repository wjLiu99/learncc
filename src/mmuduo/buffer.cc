#include "buffer.h"
#include <unistd.h>
#include <sys/uio.h>
#include <errno.h>

ssize_t buffer::read_fd (int fd, int *saveerrno) {
    char extra[65536] = {0};
    struct iovec vec[2];
    const size_t writable = writable_bytes();
    vec[0].iov_base = begin_write();
    vec[0].iov_len = writable;

    vec[1].iov_base = extra;
    vec[1].iov_len = sizeof extra;

    const int iovcnt = (writable < sizeof extra) ? 2 : 1;
    const ssize_t n = readv(fd, vec, iovcnt);
    if (n < 0) {
        *saveerrno = errno;
        return n;
    }

    if (n <= writable) {
        write_ += n;
        return n;
    }

    //extra中也有数据
    write_ = buf_.size();
    append(extra, n - writable);
    return n;


}

ssize_t buffer::write_fd (int fd, int *saveerrno) {
    ssize_t n = write(fd, peek(), readable_bytes());
    if (n < 0) {
        *saveerrno = errno;
    }
    return n;
}