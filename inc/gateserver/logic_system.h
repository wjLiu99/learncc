#ifndef _LOGIC_SYSTEM_H_
#define _LOGIC_SYSTEM_H_

#include "comm.h"
// 逻辑系统，根据消息id查找处理函数表调用
// 不要在头文件中互相包含，只在头文件中声明类，在源文件中包含
class http_conn;
typedef std::function<void(std::shared_ptr<http_conn>)> http_handler;

class logic_system : public Singleton<logic_system> {
    friend class Singleton<logic_system>;
public:
    ~logic_system (){}
    //获取请求处理函数
    bool get_handler (std::string, std::shared_ptr<http_conn>);
    bool post_handler (std::string, std::shared_ptr<http_conn>);
    //注册get请求处理函数
    void reg_gethandler (std::string, http_handler);
    void reg_posthandler (std::string, http_handler);
private:
    logic_system ();
    std::map<std::string, http_handler> post_handlers_;
    std::map<std::string, http_handler> get_handlers_;

};
#endif