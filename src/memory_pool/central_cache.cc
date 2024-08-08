#include "central_cache.h"
#include "page_cache.h"

span *central_cache::get_one_span (span_list *spanlist, size_t bytes) {
    span *s = spanlist->begin();
    // central cache的span 空闲链表中不为空，直接返回
    while (s != spanlist->head()) {
        if (nullptr != s->block_list_) {
            return s;
        }
        s = s->next_;
    }

    // 空闲链表中没有可用的内存块，需要从page cache中申请
    size_t pages = size_class::move_page(bytes);
    span *new_span = page_cache::get_instance().new_span(pages);

    // 根据页号计算出内存地址起始地址和结束地址
    char *start = (char *)(new_span->page_id_ << PAGE_SHIFT);
    char *end = start + (new_span->npages_ << PAGE_SHIFT);

    char *next = start + bytes;
    char *cur = start;

    while (next < end) {
        next_block(cur) = next;
        cur = next;
        next += bytes;
    }
    next_block(cur) = nullptr;
    // 申请到central cache才将多页内存分成多块bytes大小的内存块串联起来
    new_span->block_list_ = start;
    new_span->block_size_ = bytes;
    new_span->used_count_ = 0;

    spanlist->push_front(new_span);
    return new_span;

}

size_t central_cache::fetch_range_block (void *&start, void *&end, size_t num, size_t bytes) {
    size_t idx = size_class::index(bytes);
    span_list *spanlist = &span_list_[idx];

    std::lock_guard<std::mutex> lk(spanlist->mtx_);
    span *span = get_one_span(spanlist, bytes);

    void *cur = span->block_list_;
    void *pre = cur;
    size_t fetchnum = 0;
    // 从span中分配内存块，如果内存不够num块，有多少返回多少
    while (cur != nullptr && fetchnum < num) {
        pre = cur;
        cur = next_block(cur);
        fetchnum++;
    }
    // 切分span
    start = span->block_list_;
    end = pre;
    next_block(end) = nullptr;

    span->block_list_ = cur;
    // 增加span的引用计数，一共分配了多少块内存
    span->used_count_ += fetchnum;
    // 若span的内存已经分配完，插入队尾增加下次分配的效率
    if (nullptr == span->block_list_) {
        spanlist->erase(span);
        spanlist->push_back(span);
    }
    return fetchnum;
}

void central_cache::release_to_spans (void *start, size_t bytes) {

    size_t idx = size_class::index(bytes);
    span_list *spanlist = &span_list_[idx];

    std::lock_guard<std::mutex> lk(spanlist->mtx_);

    while (start) {
        void *next = next_block(start);

        span *s = page_cache::get_instance().block_to_span(start);
        // 将内存块从新插入span中
        next_block(start) = s->block_list_;
        s->block_list_ = start;
        // 使用计数减为0说明该span的所有内存块全部归还
        // 将引用计数减为0的span归还给page cache可以减少内存碎片
        // page cache会对内存进行合并，防止出现过多太小的空间难以利用
        if (--s->used_count_ == 0) {
            spanlist->erase(s);
            s->block_list_ = nullptr;
            s->block_size_ = 0;

            page_cache::get_instance().release_to_pagecache(s);

        }
        start = next;

    }
}