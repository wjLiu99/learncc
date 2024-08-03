#include "current_thread.h"
namespace current_thread
{
    __thread int cached_tid = 0;   

    void cache_tid()
    {
        if (cached_tid == 0)
        {
            // 通过linux系统调用，获取当前线程的tid值
            cached_tid = static_cast<pid_t>(::syscall(SYS_gettid));
        }
    }
}