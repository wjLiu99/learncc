#ifndef _ARPC_H_
#define _ARPC_H_
#include "arpc_conf.h"

#define LOG_LEVEL INFO
// arpc初始化操作
class arpc {
public:
    static void init (int argc, char **argv);
    static arpc &get_instance ();
    static arpc_conf &get_conf ();

private:
    arpc () {}
    arpc (const arpc &) = delete;
    arpc & operator= (const arpc &) = delete;

    static arpc_conf conf_;

};

#endif