#ifndef _CURRENT_THREAD_H
#define _CURRENT_THREAD_H

#include <unistd.h>
#include <sys/syscall.h>

namespace current_thread
{
    extern __thread int cached_tid;

    void cache_tid();

    inline int tid()
    {
        if (__builtin_expect(cached_tid == 0, 0))
        {
            cache_tid();
        }
        return cached_tid;
    }
}

#endif