#include "logic_system.h"
#include "http_conn.h"
#include "verify_grpc_client.h"
#include "msg.pb.h"
#include "redis_mgr.h"

void logic_system::reg_gethandler (std::string url, http_handler handler) {
    get_handlers_.insert(std::make_pair(url, handler));
}
 
void logic_system::reg_posthandler (std::string url, http_handler handler) {
    post_handlers_.insert(std::make_pair(url, handler));
}

logic_system::logic_system () {
    reg_gethandler("/get_test", [](std::shared_ptr<http_conn> conn) {
        beast::ostream(conn->res_.body()) << "receive get rest req";
        int i = 0;
        for (auto &elem : conn->get_params_) {
            i++;
            beast::ostream(conn->res_.body()) << "para " << i << "key is " << elem.first << 
            "value is " << elem.second << std::endl;
        }
    });
    
    reg_posthandler("/get_verifycode", [](std::shared_ptr<http_conn> conn){
        auto str = boost::beast::buffers_to_string(conn->req_.body().data());
        std::cout << "receive body is " << str << std::endl;
        conn->res_.set(http::field::content_type, "text/json");
        Json::Value root;
        Json::Reader reader;
        Json::Value src_root;
        bool is_parse_ok = reader.parse(str, src_root);
        if (!is_parse_ok || !src_root.isMember("email")) {
            std::cout << "parse json err" << std::endl;
            root["err"] = ERR_JSON;
            std::string jsonstr = root.toStyledString();
            beast::ostream(conn->res_.body()) << jsonstr;
            return ;
        }
        
        auto email = src_root["email"].asString();

        get_verify_rsp rsp = verify_grpc_client::get_instance()->get_verify_code(email);


        std::cout << "email is :" << email << std::endl;
        root["err"] = rsp.error();
        root["email"] = src_root["email"];
        std::string jsonstr = root.toStyledString();
        beast::ostream(conn->res_.body()) << jsonstr;
        return;

    });


    reg_posthandler("/user_register", [](std::shared_ptr<http_conn> connection) {
    auto body_str = boost::beast::buffers_to_string(connection->req_.body().data());
    std::cout << "receive body is " << body_str << std::endl;
    connection->res_.set(http::field::content_type, "text/json");
    Json::Value root;
    Json::Reader reader;
    Json::Value src_root;
    bool parse_success = reader.parse(body_str, src_root);
    if (!parse_success) {
        std::cout << "Failed to parse JSON data!" << std::endl;
        root["err"] = ERR_JSON;
        std::string jsonstr = root.toStyledString();
        beast::ostream(connection->res_.body()) << jsonstr;
        return true;
    }
    //先查找redis中email对应的验证码是否合理
    std::string  verify_code;
    bool b_get_verify = redis_mgr::get_instance()->get(CODEPREFIX + src_root["email"].asString(), verify_code);
    if (!b_get_verify) {
        std::cout << " get verify code expired" << std::endl;
        root["err"] = ERR_VER_EXPIRED;
        std::string jsonstr = root.toStyledString();
        beast::ostream(connection->res_.body()) << jsonstr;
        return true;
    }
    if (verify_code != src_root["verifycode"].asString()) {
        std::cout << " verify code err" << std::endl;
        root["err"] = ERR_VER_CODE;
        std::string jsonstr = root.toStyledString();
        beast::ostream(connection->res_.body()) << jsonstr;
        return true;
    }
    //访问redis查找 判断用户是否存在
    bool b_usr_exist = redis_mgr::get_instance()->exist_key(src_root["user"].asString());
    if (b_usr_exist) {
        std::cout << " user exist" << std::endl;
        root["err"] = ERR_EXIST;
        std::string jsonstr = root.toStyledString();
        beast::ostream(connection->res_.body()) << jsonstr;
        return true;
    }
    
    root["err"] = 0;
    root["email"] = src_root["email"];
    root ["user"]= src_root["user"].asString();
    root["pwd"] = src_root["pwd"].asString();
    root["confirm"] = src_root["confirm"].asString();
    root["verifycode"] = src_root["verifycode"].asString();
    std::string jsonstr = root.toStyledString();
    beast::ostream(connection->res_.body()) << jsonstr;
    return true;

    });
}

bool logic_system::get_handler(std::string path, std::shared_ptr<http_conn> conn) {
    if (get_handlers_.end() == get_handlers_.find(path)) {
        return false;
    }
    get_handlers_[path](conn);
    return true;
}


bool logic_system::post_handler(std::string path, std::shared_ptr<http_conn> conn) {
    if (post_handlers_.end() == post_handlers_.find(path)) {
        return false;
    }
    post_handlers_[path](conn);
    return true;
}