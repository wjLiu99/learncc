#ifndef _ARPC_CONTROLLER_H_
#define _ARPC_CONTROLLER_H_

#include <google/protobuf/service.h>
#include <string>


class arpc_controller : public google::protobuf::RpcController
{
public:
    arpc_controller();
    // Client-side methods ---------------------------------------------
    void Reset();
    bool Failed() const;
    void StartCancel();
    std::string ErrorText() const;
    
    // Server-side methods ---------------------------------------------
    void SetFailed(const std::string& reason);
    bool IsCanceled() const;
    void NotifyOnCancel(google::protobuf::Closure* callback);
private:
    bool failed_; // RPC方法执行过程中的状态
    std::string err_text_; // RPC方法执行过程中的错误信息
};
#endif