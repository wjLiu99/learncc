#ifndef _NGX_MEMPOOL_H_
#define _NGX_MEMPOOL_H_
#include <cstddef>

using u_char = unsigned char;
using ngx_uint_t = unsigned int;
using ngx_int_t = long long;

#define ngx_align(d, a)     (((d) + (a - 1)) & ~(a - 1))
#define ngx_align_ptr(p, a)                                                   \
    (u_char *) (((ngx_int_t) (p) + ((ngx_int_t) a - 1)) & ~((ngx_int_t) a - 1))

#define NGX_ALIGNMENT   sizeof(unsigned long) //字节对齐单位

const int ngx_pagesize = 4096;
#define NGX_MAX_ALLOC_FROM_POOL  (ngx_pagesize - 1)
// 默认内存池大小
#define NGX_DEFAULT_POOL_SIZE    (16 * 1024)
// 默认16字节对齐
#define NGX_POOL_ALIGNMENT       16
#define NGX_MIN_POOL_SIZE                                                     \
    ngx_align((sizeof(ngx_pool_t) + 2 * sizeof(ngx_pool_large_t)), NGX_POOL_ALIGNMENT)// 向上对齐


typedef void (*ngx_pool_cleanup_pt)(void *data);

struct ngx_pool_t;
// 清理外部空间
struct ngx_pool_cleanup_t {
    ngx_pool_cleanup_pt   handler;
    void                 *data;
    ngx_pool_cleanup_t   *next;
};

// 大块内存头部信息
struct ngx_pool_large_t {
    ngx_pool_large_t     *next;
    void                 *alloc;
};

// 分配小块内存头部信息
typedef struct {
    u_char               *last;  // 起始地址
    u_char               *end;   //内存末尾地址   
    ngx_pool_t           *next;     //链表结构
    ngx_uint_t            failed;   // 分配失败次数
} ngx_pool_data_t;

// 内存池头部信息和管理信息
struct ngx_pool_t {
    ngx_pool_data_t       d;
    size_t                max;      // 大小内存分界线
    ngx_pool_t           *current;  // 第一个小块内存
   // ngx_chain_t          *chain;
    ngx_pool_large_t     *large;    // 大块内存入口
    ngx_pool_cleanup_t   *cleanup;  //  清理回调函数的入口
 //   ngx_log_t            *log;
};

#define ngx_memzero(buf, n)       (void) memset(buf, 0, n)

class ngx_mempool
{
private:
    ngx_pool_t *pool_;
    void *ngx_palloc_small(size_t size, ngx_uint_t align);
    void *ngx_palloc_large(size_t size);
    void *ngx_palloc_block(size_t size);
public:
    void *ngx_create_pool(size_t size);
    void *ngx_palloc(size_t size);
    void *ngx_pnalloc(size_t size);
    void *ngx_pcalloc(size_t size);
    void ngx_pfree(ngx_pool_t *pool, void *p);
    ngx_pool_cleanup_t *ngx_pool_cleanup_add(size_t size);
    void ngx_reset_pool();
    void ngx_destroy_pool();

};



#endif