#ifndef _THREAD_CACHE_H_
#define _THREAD_CACHE_H_

#include "comm.h"
#include "central_cache.h"

class thread_cache {

private :
    // 空闲内存块链表
    free_list free_list_[NLISTS];

public:
    void *allocate (size_t size);
    void deallocate (void *ptr, size_t size);

    // 从central cache中获取内存块
    void *fetch_from_centralcache (size_t index, size_t size);

    // 空闲链表太长时归还给central cache
    void list_too_long (free_list *freelist, size_t byte);
};

static thread_local  thread_cache* tls_threadcache = nullptr;
#endif