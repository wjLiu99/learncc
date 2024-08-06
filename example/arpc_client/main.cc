#include "user.pb.h"
#include "arpc.h"
#include "arpc_channel.h"
#include <atomic>
std::atomic<int> cnt(0);
static void done (login_response *response) {
    cnt.fetch_add(1, std::memory_order_seq_cst);
    if (0 == response->result().errcode())
    {
        std::cout << "rpc login response success:" << response->sucess() << std::endl;
    }
    else
    {
        std::cout << "rpc login response error : " << response->result().errmsg() << std::endl;
    }
    std::cout << "cnt : " << cnt.load(std::memory_order_seq_cst) << std::endl;

}
int main(int argc, char const *argv[])
{
    user_service_Stub st(new arpc_channel);

    login_request req;
    req.set_name("liu wen jie");
    req.set_pwd("123456");
    for (int i = 0; i < 300; ++i){
        login_response *rsp = new login_response();
        st.login(nullptr, &req, rsp, google::protobuf::NewCallback(done, rsp));
    }
    std::cout << "done" << std::endl;
    std::cout << "done" << std::endl;
    std::cout << "done" << std::endl;
    std::cout << "done" << std::endl;
    std::cout << "done" << std::endl;
    std::cout << "done" << std::endl;
    std::cout << "done" << std::endl;

    std::cout << "done" << std::endl;
    std::cout << "done" << std::endl;
    while (1);
    return 0;
}
