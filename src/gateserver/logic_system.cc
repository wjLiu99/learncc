#include "logic_system.h"
#include "http_conn.h"


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
        std::cout << "email is :" << email << std::endl;
        root["err"] = SUCCESS;
        root["email"] = src_root["email"];
        std::string jsonstr = root.toStyledString();
        beast::ostream(conn->res_.body()) << jsonstr;
        return;

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