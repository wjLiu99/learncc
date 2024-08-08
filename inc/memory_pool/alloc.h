#ifndef _ALLOC_H_
#define _ALLOC_H_

#include "comm.h"
#include "thread_cache.h"
#include "page_cache.h"
#include "mutex"

std::mutex mtx;

void *tc_malloc (size_t size) {
    if (size > MAXBYTES) {
        size = size_class::round_up(size, 1 << PAGE_SHIFT);
        size_t pages = size >> PAGE_SHIFT;

        span *s = nullptr;
        {
            std::lock_guard<std::mutex> lk(mtx);
			
            s = page_cache::get_instance().new_span(pages);

			s->used_count_++;
        }
        void *ptr = (void *)(s->page_id_ << PAGE_SHIFT);
        return ptr;
    }

    if (nullptr == tls_threadcache) {
        tls_threadcache = new thread_cache;
    }
    return tls_threadcache->allocate(size);
}


void tc_free (void *ptr) {
    span *s = page_cache::get_instance().block_to_span(ptr);

    size_t size = s->block_size_;
    if (size > MAXBYTES) {
        std::lock_guard<std::mutex> lk(mtx);
		s->used_count_--;
		page_cache::get_instance().release_to_pagecache(s);
    } else {
        return tls_threadcache->deallocate(ptr, size);
    }
}
#endif