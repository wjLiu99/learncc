#include "ngx_mempool.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef struct Data stData;
struct Data
{
    char *ptr;
    FILE *pfile;
};

void func1(char *p)
{
    printf("free ptr mem!");
    free(p);
}
void func2(FILE *pf)
{
    printf("close file!");
    fclose(pf);
}
void ngx_test()
{
	// 512 - sizeof(ngx_pool_t) - 4095   =>   max
    ngx_mempool pool;
    // 构造
    ngx_pool_t *s = (ngx_pool_t *)pool.ngx_create_pool(512);
    if(s == nullptr)
    {
        printf("ngx_create_pool fail...");
        return;
    }

    void *p1 = pool.ngx_palloc(128); // 从小块内存池分配的
    if(p1 == nullptr)
    {
        printf("ngx_palloc 128 bytes fail...");
        return;
    }

    stData *p2 = (stData *)pool.ngx_palloc(512); // 从大块内存池分配的
    if(p2 == NULL)
    {
        printf("ngx_palloc 512 bytes fail...");
        return;
    }
    p2->ptr = (char *)malloc(12);
    strcpy(p2->ptr, "hello world");
    p2->pfile = fopen("data.txt", "w");
    //可以使用函数绑定器
    ngx_pool_cleanup_t *c1 = pool.ngx_pool_cleanup_add(sizeof(char*));
    c1->handler = (ngx_pool_cleanup_pt)func1;
    c1->data = p2->ptr;

    ngx_pool_cleanup_t *c2 = pool.ngx_pool_cleanup_add(sizeof(FILE*));
    c2->handler = (ngx_pool_cleanup_pt)func2;
    c2->data = p2->pfile;
    //析构
    pool.ngx_destroy_pool(); // 1.调用所有的预置的清理函数 2.释放大块内存 3.释放小块内存池所有内存

    return;
}
