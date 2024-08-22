#ifndef _PAGE_CACHE_H_
#define _PAGE_CACHE_H_

#include "comm.h"
#include "ts_hash_tbl.h"
#include "ts_list.h"
// page cache为单例模式，多线程访问时要加锁
// central cache从空闲的page列表中获取span

class page_cache {
private:
    page_cache () {};
    page_cache (const page_cache &) = delete;
    page_cache & operator= (const page_cache &) = delete;
    span *new_span_ (size_t pages);
    // 空闲的span链表， NPAGES是129，只用1 - 128表示该链表中span包含的页数
    // page_list[10] 表示这条链表中所有的span都包含10页内存空间
    span_list page_list_[NPAGES];

    std::mutex mtx_;
    std::unordered_map<page_id, span *> pageid_span_map_;
    // ts_hash_tbl<page_id, span *> pageid_span_map_;
    

public:
    static page_cache &get_instance () {
        static page_cache instance;
        return instance;
    }
    // 申请新的span，空闲链表中没有就向系统申请
    span *new_span (size_t pages);


    // 根据内存地址映射到页id，找到对应的span
    span *block_to_span (void *ptr);
    // 归还申请的span
    void release_to_pagecache (span *s);

};
#endif