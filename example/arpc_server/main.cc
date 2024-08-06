#include "user.pb.h"
#include "arpc.h"
#include "arpc_server.h"
#include <thread>
class local_user_service : public user_service // 使用在rpc服务发布端（rpc服务提供者）
{
public:
    bool Login(std::string name, std::string pwd)
    {
        std::cout << "doing local service: Login" << std::endl;
        std::cout << "name:" << name << " pwd:" << pwd << std::endl;  
        std::this_thread::sleep_for(std::chrono::microseconds(10));
        return false;
    }


    /*
    重写基类UserServiceRpc的虚函数 下面这些方法都是框架直接调用的
    1. caller   ===>   Login(LoginRequest)  => muduo =>   callee 
    2. callee   ===>    Login(LoginRequest)  => 交到下面重写的这个Login方法上了
    */
    void login(::google::protobuf::RpcController* controller,
                       const ::login_request* request,
                       ::login_response* response,
                       ::google::protobuf::Closure* done)
    {
        // 框架给业务上报了请求参数LoginRequest，应用获取相应数据做本地业务
        std::string name = request->name();
        std::string pwd = request->pwd();

        // 做本地业务
        bool login_result = Login(name, pwd); 

        // 把响应写入  包括错误码、错误消息、返回值
        ::result_code *code = response->mutable_result();
        code->set_errcode(1);
        code->set_errmsg("login failed");
        response->set_sucess(login_result);

        // 执行回调操作   执行响应对象数据的序列化和网络发送（都是由框架来完成的）
        done->Run();
    }
};

int main(int argc, char const *argv[])
{
   
    arpc_server server;
    server.register_service(new local_user_service);
    server.start();
    return 0;
}
