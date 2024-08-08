#include "thread_cache.h"
#include "central_cache.h"

void *thread_cache::allocate (size_t bytes) {
    assert(bytes < MAXBYTES);
    // 向上对齐申请字节数，找到对应列表申请，不空直接返回，若为空去central cache申请
    bytes = size_class::round_up(bytes);
    size_t idx = size_class::index(bytes);
    free_list *freelist = &free_list_[idx];

    if (!freelist->empty()) {
        return freelist->pop();
    }

    // 若线程的自由链表为空，从central cache申请内存块，一次取多块防止频繁申请加锁开销
    // 申请数量遵循慢启动策略，随着申请次数增加而增加，防止一次获取过多而其他线程需要获取就要去page cache申请新的span带来而外开销
    return fetch_from_centralcache(idx, bytes);
}

void thread_cache::deallocate (void *ptr, size_t bytes) {
    assert(bytes < MAXBYTES);
    // 找到对应链表插入
    size_t idx = size_class::index(bytes);
    free_list *freelist = &free_list_[idx];
    freelist->push(ptr);

    // 资源回收策略，但自由链表长度超过一次从central cache中申请的长度时，将链表中的空闲内存块回收给central cache
    if (freelist->size() >= freelist->max_size()) {
        list_too_long(freelist, bytes);
    }
}
// 归还空闲链表中所有的内存块
void thread_cache::list_too_long (free_list *fl, size_t bytes) {
    void *start = fl->clear();
    central_cache::get_instance().release_to_spans(start, bytes);
}


void *thread_cache::fetch_from_centralcache (size_t index, size_t bytes) {
    free_list *freelist = &free_list_[index];
    // 本次申请的内存块数量，取链表容量和move num计算的最小值
    // 计算规则 申请内存越小，数量越多，申请内存越大，数量越小
    // 申请次数越多，maxsize越大，单次能申请的数量也就越多
    size_t num = std::min(size_class::move_num(bytes), freelist->max_size());

    void *start, *end;
    size_t fetchnum = central_cache::get_instance().fetch_range_block(start, end, num, bytes);
    if (fetchnum > 1) {
        freelist->push_range(next_block(start), end, fetchnum - 1);
    }
    if (num == freelist->max_size()) {
        freelist->set_maxsize(num + 1);
    }
    return start;
}
