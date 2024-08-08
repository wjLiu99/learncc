#include "ngx_mempool.h"
#include <cstdlib>
#include <string.h>
void *ngx_mempool::ngx_palloc_small(size_t size, ngx_uint_t align) {
    u_char      *m;
    ngx_pool_t  *p;

    p = pool_->current;

    do {
        m = p->d.last;

        if (align) {
            m = ngx_align_ptr(m, NGX_ALIGNMENT);
        }

        if ((size_t) (p->d.end - m) >= size) {
            p->d.last = m + size;

            return m;
        }

        p = p->d.next;

    } while (p);

    return ngx_palloc_block(size);
}
void *ngx_mempool::ngx_palloc_large(size_t size) {
    void              *p;
    ngx_uint_t         n;
    ngx_pool_large_t  *large;

    p = malloc(size);
    if (p == nullptr) {
        return nullptr;
    }

    n = 0;

    for (large = pool_->large; large; large = large->next) {
        if (large->alloc == nullptr) {
            large->alloc = p;
            return p;
        }
        //超过三次不找了
        if (n++ > 3) {
            break;
        }
    }

    large = (ngx_pool_large_t *)ngx_palloc_small(sizeof(ngx_pool_large_t), 1);
    if (large == nullptr) {
        free(p);
        return nullptr;
    }

    large->alloc = p;
    large->next = pool_->large;
    pool_->large = large;

    return p;
}
void *ngx_mempool::ngx_palloc_block(size_t size) {
    u_char      *m;
    size_t       psize;
    ngx_pool_t  *p, *newpool;

    psize = (size_t) (pool_->d.end - (u_char *) pool_);

    m = (u_char *)malloc(psize);
    if (m == nullptr) {
        return nullptr;
    }

    newpool = (ngx_pool_t *) m;

    newpool->d.end = m + psize;
    newpool->d.next = nullptr;
    newpool->d.failed = 0;

    m += sizeof(ngx_pool_data_t);
    m = ngx_align_ptr(m, NGX_ALIGNMENT);
    newpool->d.last = m + size;

    for (p = pool_->current; p->d.next; p = p->d.next) {
        if (p->d.failed++ > 4) {
            pool_->current = p->d.next;
        }
    }

    p->d.next = newpool;

    return m;
}

void *ngx_mempool::ngx_create_pool(size_t size) {
        ngx_pool_t  *p;

    p = (ngx_pool_t *)malloc(size);
    if (p == nullptr) {
        return nullptr;
    }

    p->d.last = (u_char *) p + sizeof(ngx_pool_t);
    p->d.end = (u_char *) p + size;
    p->d.next = nullptr;
    p->d.failed = 0;

    size = size - sizeof(ngx_pool_t);
    p->max = (size < NGX_MAX_ALLOC_FROM_POOL) ? size : NGX_MAX_ALLOC_FROM_POOL;

    p->current = p;

    p->large = nullptr;
    p->cleanup = nullptr;

    pool_ = p;
    return p;

}
// 考虑对齐分配内存
void *ngx_mempool::ngx_palloc(size_t size) {

    if (size <= pool_->max) {
        return ngx_palloc_small(size, 1);
    }
    return ngx_palloc_large(size);
}
// 不考虑内存对齐
void *ngx_mempool::ngx_pnalloc(size_t size) {
    if (size <= pool_->max) {
        return ngx_palloc_small(size, 0);
    }
    return ngx_palloc_large(size);
}
// 分配内存并清零
void *ngx_mempool::ngx_pcalloc(size_t size) {
    void *p;

    p = ngx_palloc(size);
    if (p) {
        ngx_memzero(p, size);
    }

    return p;
}
void ngx_mempool::ngx_pfree(ngx_pool_t *pool, void *p) {
    ngx_pool_large_t  *l;

    for (l = pool->large; l; l = l->next) {
        if (p == l->alloc) {
           
            free(l->alloc);
            l->alloc = NULL;

            return ;
        }
    }

    return ;
}
ngx_pool_cleanup_t *ngx_mempool::ngx_pool_cleanup_add(size_t size) {
    ngx_pool_cleanup_t  *c;

    c = (ngx_pool_cleanup_t *)ngx_palloc(sizeof(ngx_pool_cleanup_t));
    if (c == NULL) {
        return NULL;
    }

    if (size) {
        c->data = ngx_palloc(size);
        if (c->data == nullptr) {
            return nullptr;
        }

    } else {
        c->data = nullptr;
    }

    c->handler = nullptr;
    c->next = pool_->cleanup;

    pool_->cleanup = c;

  

    return c;
}
void ngx_mempool::ngx_reset_pool() {
    ngx_pool_t        *p;
    ngx_pool_large_t  *l;

    for (l = pool_->large; l; l = l->next) {
        if (l->alloc) {
            free(l->alloc);
        }
    }

    // for (p = pool_; p; p = p->d.next) {
    //     //这里不对,除了第一个内存块后面的内存块只有数据头信息，没有内存池控制信息
    //     //浪费了一部分空间
    //     p->d.last = (u_char *) p + sizeof(ngx_pool_t);
    //     p->d.failed = 0;
    // }

    p = pool_;
    p->d.last = (u_char *)p + sizeof (ngx_pool_t);
    p->d.failed = 0;
    // 第二块内存池开始循环到最后一个内存池
    for (p = p->d.next; p; p = p->d.next) {
        p->d.last = (u_char *)p + sizeof(ngx_pool_data_t);
        p->d.failed = 0;
    }

    
    pool_->current = pool_;
    pool_->large = nullptr;
}
void ngx_mempool::ngx_destroy_pool() {
    ngx_pool_t          *p, *n;
    ngx_pool_large_t    *l;
    ngx_pool_cleanup_t  *c;

    for (c = pool_->cleanup; c; c = c->next) {
        if (c->handler) {
            // ngx_log_debug1(NGX_LOG_DEBUG_ALLOC, pool->log, 0,
            //                "run cleanup: %p", c);
            c->handler(c->data);
        }
    }


    for (l = pool_->large; l; l = l->next) {
        if (l->alloc) {
            free(l->alloc);
        }
    }

    for (p = pool_, n = pool_->d.next; /* void */; p = n, n = n->d.next) {
        free(p);

        if (n == NULL) {
            break;
        }
    }
}