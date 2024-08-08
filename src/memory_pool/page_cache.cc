#include "page_cache.h"

span *page_cache::new_span (size_t pages) {
    std::lock_guard<std::mutex> lk(mtx_);
    // 申请大于128页内存直接去系统申请 128 * 4096
    if (pages >= NPAGES) {
        void *ptr = sys_alloc(pages);
        span *s = new span;
        s->page_id_ = (page_id)ptr >> PAGE_SHIFT;
        s->npages_ = pages;
        s->block_size_ = pages << PAGE_SHIFT;

        // 申请大块内存时只用记录第一个页号，不会经过central cache的拆分，所以不需要这个span其他页的映射关系
        // 归还的时候是直接归还给page cache 
        pageid_span_map_[s->page_id_] = s;
        return s;
    }
    // 申请小于128页的内存时
    span *s = new_span_(pages);
    // 将内存归还给操作系统时需要内存块大小
    s->block_size_ = s->npages_ << PAGE_SHIFT;
    return s;
}

span *page_cache::new_span_ (size_t pages) {
    // 如果span空闲链表中有未使用的span直接返回
    if (!page_list_[pages].empty()) {
        return page_list_[pages].pop_front();
    }
    // 若pages页数的span链表为空，则往后变量比pages大的span链表进行切割，遍历到最后没有找到就向系统申请

    for (size_t i = pages + 1; i < NPAGES; ++i) {
        if (!page_list_[i].empty()) {
            span *s = page_list_[i].pop_front();
            span *split = new span;
            // 从span尾部切割
            split->page_id_ = s->page_id_ + s->npages_ - pages;
            split->npages_ = pages;
            s->npages_ = s->npages_ - pages;
            // 重新建立页号和span的映射关系
            for (size_t i = 0; i < pages; ++i) {
                pageid_span_map_[split->page_id_ + i] = split;

            }
            // 剩下的span重新插入空闲链表
            page_list_[s->npages_].push_front(s);
            return split;
        }
    }

    // 没有合适的span切割，重新向系统申请
    void *ptr = sys_alloc(NPAGES - 1);
    span *newspan = new span;
    newspan->page_id_ = (page_id)(ptr) >> PAGE_SHIFT;
    newspan->npages_ = NPAGES - 1;
    page_list_[NPAGES - 1] .push_front(newspan);
    // 将新申请的span建立页号的映射关系
    for (size_t i = 0; i < newspan->npages_; ++i) {
        pageid_span_map_[newspan->page_id_ + i] = newspan;
    }
    // 递归一次
    return new_span_(pages);

}

span *page_cache::block_to_span (void *ptr) {
    page_id id = (page_id)(ptr) >> PAGE_SHIFT;
    auto it = pageid_span_map_.find(id);
    assert (it != pageid_span_map_.end());

    return it->second;
}

// 将central cache中span归还给page cache进行合并，减少外部碎片
void page_cache::release_to_pagecache (span *s) {
    std::lock_guard<std::mutex> lk(mtx_);

    if (s->npages_ >= NPAGES) {
        void *ptr = (void *)(s->page_id_ << PAGE_SHIFT);
        pageid_span_map_.erase(s->page_id_);
        sys_free(ptr, s->npages_);
        delete s;
        return;
    }

    auto pre_it = pageid_span_map_.find(s->page_id_ - 1);
    while (pageid_span_map_.end() != pre_it) {
        span *pre_span = pre_it->second;

        if (pre_span->used_count_ != 0) {
            break;;
        }

        if (pre_span->npages_ + s->npages_ >= NPAGES) {
            break;
        }

        page_list_[pre_span->npages_].erase(pre_span);
        pre_span->npages_ += s->npages_;
        delete(s);
        s = pre_span;
        pre_it = pageid_span_map_.find(s->page_id_ - 1);
    }

    auto next_it = pageid_span_map_.find(s->page_id_ + s->npages_);
    while (pageid_span_map_.end() != next_it) {
        span *next_span = next_it->second;

        if (next_span->used_count_ != 0) {
            break;;
        }

        if (next_span->npages_ + s->npages_ >= NPAGES) {
            break;
        }
        // 合并到前一个span中
        page_list_[next_span->npages_].erase(next_span);
        s->npages_ += next_span->npages_;
        delete(next_span);
     
        next_it = pageid_span_map_.find(s->page_id_ + s->npages_);
    }

    // 将合并好的span建立页映射关系
    for (size_t i = 0; i < s->npages_; ++i) {
        pageid_span_map_[s->page_id_ + i] = s;
    }
    // 将合并好的span插入到空闲span链表中，每次归还如果不能合并也要插入到空闲链表中
    page_list_[s->npages_].push_front(s);
    
}