#include "arpc_server.h"
#include "synclog.h"
#include "arpc.h"
#include "arpc_header.pb.h"
using namespace std;
using namespace std::placeholders;
using namespace google::protobuf;
using namespace muduo::net;
using namespace muduo;
void arpc_server::register_service (Service *service) {
    service_info service_info;


    const ServiceDescriptor *desc = service->GetDescriptor();
    std::string service_name = desc->name();
    int method_cnt = desc->method_count();

    LOG_INFO(LOG_LEVEL, "service name : ", service_name);

    for (int i = 0; i < method_cnt; ++i) {
        const MethodDescriptor *m_desc = desc->method(i);
        string method_name = m_desc->name();
        service_info.method_map_.emplace(method_name, m_desc);
        LOG_INFO(LOG_LEVEL, "method name : ", method_name);
    }
    service_info.service_ = service;
    service_map_.emplace(service_name, service_info);
}

void arpc_server::start () {
    // string ip = arpc::get_instance().get_conf().load("server_ip");
    // uint16_t port = atoi(arpc::get_instance().get_conf().load("server_port").c_str());
    // InetAddress addr("127.0.0.1", 8000);
    // TcpServer server(&loop_, addr, "arpc_server");

    inet_address addr(8000, "127.0.0.1");
    tcp_server server(&loop_, addr, "arpc_server");

    server.set_connection_cb(
        std::bind(&arpc_server::on_connection, this, _1)
    );
    server.set_message_cb(
        std::bind(&arpc_server::on_message, this, _1, _2, _3)
    );

    thread_num_ = (thread_num_ == 0) ? std::thread::hardware_concurrency() : thread_num_;
    server.set_threadnum(1);
    // 启动线程池，acceptor开始listen
    server.start();
    // 开始事件循环
    loop_.loop();


}

void arpc_server::on_connection (const tcp_connection_ptr &conn) {
    if (!conn->connected()) {
        conn->shutdown();
    }
}

void arpc_server::on_message(const tcp_connection_ptr &conn, 
                            buffer *buffer, 
                            timestamp) {
    // 读4字节头信息
    string recv_buf = buffer->retrieve_as_string(4);
    uint32_t hdr_size = 0;
    recv_buf.copy((char *)&hdr_size, 4, 0);

    // 读取rpc请求头
    string arpc_hdr_str = buffer->retrieve_as_string(hdr_size);
    arpc_header arpc_hdr;
    // 序列化失败
    if (!arpc_hdr.ParseFromString(arpc_hdr_str)) {
        LOG_ERROR(LOG_LEVEL, "rpc header str :", arpc_hdr_str, " parse failed");
        return;
    }

    string service_name = arpc_hdr.service_name();
    string method_name = arpc_hdr.method_name();
    uint32_t args_size = arpc_hdr.args_size();
    uint32_t id = arpc_hdr.id();

    string args_str = buffer->retrieve_as_string(args_size);
    LOG_INFO(LOG_LEVEL, "============================================\n", 
    "header_size : {}\n", hdr_size, 
    "arpc_header_str : {}\n", arpc_hdr_str,
    "service_name : {}\n", service_name,
    "method_name : {}\n", method_name,
    "args_str : {}\n", args_str, "============================================\n");

    auto it = service_map_.find(service_name);
    if (service_map_.end() == it) {
        LOG_ERROR(LOG_LEVEL, service_name, "is not exist!");
        return;
    }

    auto m_it = it->second.method_map_.find(method_name);
    if (it->second.method_map_.end() == m_it) {
        LOG_ERROR(LOG_LEVEL, service_name, " : ", method_name, "is not exist!");
        return;
    }

    Service *service = it->second.service_;
    const MethodDescriptor *method = m_it->second;

    Message *request = service->GetRequestPrototype(method).New();
    if (!request->ParseFromString(args_str)) {
        LOG_ERROR(LOG_LEVEL, "request parse error, content : ", args_str);
        return;
    }
    Message *response = service->GetResponsePrototype(method).New();
    response_info res_info;
    res_info.response = response;
    res_info.id = id;
    // new callback返回一个closure对象，将参数拷贝一份存储，重写了run函数调用函数对象
    // 不会decay，传入什么类型就保存什么类型
    Closure *done = NewCallback<arpc_server, 
                                const tcp_connection_ptr&, 
                                response_info> (
                                    this,
                                    &arpc_server::send_response,
                                    conn,
                                    res_info
                                );
    service->CallMethod(method, nullptr, request, response, done);

}

void arpc_server::send_response (const tcp_connection_ptr& conn, response_info res) {
    

    string response_str;
    Message *response = res.response;

    if (!response->SerializeToString(&response_str)) {
       LOG_ERROR(LOG_LEVEL, "serialize response str error!");
       conn->shutdown();
       return;
    } 

    uint32_t args_size = response_str.size();
    arpc_header arpc_hdr;
    arpc_hdr.set_id(res.id);
    arpc_hdr.set_args_size(args_size);
    arpc_hdr.set_type(RESPONSE);


    
    string arpc_hdr_str;
    if (!arpc_hdr.SerializeToString(&arpc_hdr_str)) {
        LOG_ERROR(LOG_LEVEL, "serialize arpc hdr str error!");
        conn->shutdown();
        return;
    }

    uint32_t hdr_size = arpc_hdr_str.size();
    string send_str;
    send_str.insert(0, string((char *)&hdr_size, 4));
    send_str += arpc_hdr_str;
    send_str += response_str;
    conn->send(send_str);
    conn->shutdown();
}