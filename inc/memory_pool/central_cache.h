#ifndef _CENTRAL_CACHE_H_
#define _CENTRAL_CACHE_H_

// 均衡各个线程的内存资源，线程空闲内存过多时归还给central cache，内存不足时向central cache 申请
// 单例，多线程访问需要加锁，访问不同的span list可以并行

#include "comm.h"

class central_cache {

private:
    central_cache () = default;
    central_cache (const central_cache &) = delete;
    central_cache & operator= (const central_cache &) = delete;
    // 空闲的span列表，每个span被切分为大小相同的小内存块串联起来
    // 从page cache 申请的span被切分为相同大小的内存块串联起开，添加到对应大小的链表中
    span_list span_list_[NLISTS];

public:
    static central_cache &get_instance () {
        static central_cache ins_;
        return ins_;
    }
    // thread cache向central cache 申请一定数量的bytes大小的内存块， 传入指针的引用可以改变指针的指向，返回一片新的地址空间
    size_t fetch_range_block (void *&start, void *&end, size_t num, size_t bytes);
    // 从bytes大小的空闲span链表中获取一个span进行分配，链表为空就向page cache申请新的span
    span *get_one_span (span_list *spanlist, size_t bytes);
    // thread cache空闲内存块过多是将空间归还给central cache
    void release_to_spans (void *start, size_t byte);

};
#endif