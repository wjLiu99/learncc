#ifndef _COMM_H_
#define _COMM_H_

#define MXA_LENGTH          1024    //数据最大长度
#define HDR_TOTAL_SIZE      4       //头部大小
#define HDR_ID_SIZE         2       //头部ID大小    
#define HDR_DATA_SIZE       2       //头部数据长度
#define RECVQUE_MAX         10000
#define SENDQUE_MAX         10000
enum {
    HELLO_MSG,
};
#endif