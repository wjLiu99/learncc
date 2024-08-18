#include "logic_system.h"
#include "http_conn.h"
#include "verify_grpc_client.h"
#include "msg.pb.h"
#include "redis_mgr.h"
#include "mysql_mgr.h"
#include "status_grpc_client.h"
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>
// 注册get请求处理器
void logic_system::reg_gethandler (std::string url, http_handler handler) {
    get_handlers_.insert(std::make_pair(url, handler));
}
// 注册post请求处理器
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
    // 获取验证码
    reg_posthandler("/get_verifycode", [](std::shared_ptr<http_conn> conn){
        auto str = boost::beast::buffers_to_string(conn->req_.body().data());
        std::cout << "receive body is " << str << std::endl;
        conn->res_.set(http::field::content_type, "text/json");
        // 回复json数据
        Json::Value root;
        Json::Reader reader;
        // 接收到的json数据
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
        // 使用rpc调用verify server上的获取验证码服务
        get_verify_rsp rsp = verify_grpc_client::get_instance()->get_verify_code(email);


        std::cout << "email is :" << email << std::endl;
        root["err"] = rsp.error();
        root["email"] = src_root["email"];
        std::string jsonstr = root.toStyledString();
        beast::ostream(conn->res_.body()) << jsonstr;
        return;

    });

    // 用户注册请求
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
        // 验证码不存在
        if (!b_get_verify) {
            std::cout << " get verify code expired" << std::endl;
            root["err"] = ERR_VER_EXPIRED;
            std::string jsonstr = root.toStyledString();
            beast::ostream(connection->res_.body()) << jsonstr;
            return true;
        }
        // 验证码错误
        if (verify_code != src_root["verifycode"].asString()) {
            std::cout << " verify code err" << std::endl;
            root["err"] = ERR_VER_CODE;
            std::string jsonstr = root.toStyledString();
            beast::ostream(connection->res_.body()) << jsonstr;
            return true;
        }

        auto email = src_root["email"].asString();
        auto name = src_root["user"].asString();
        auto pwd = src_root["pwd"].asString();
        auto confirm = src_root["confirm"].asString();
        auto icon = src_root["icon"].asString();

        //查找数据库判断用户是否存在
        int uid = mysql_mgr::get_instance()->reg_user(name, email, pwd);
        // 用户已经存在
        if (uid == 0 || uid == -1) {
            std::cout << " user or email exist" << std::endl;
            root["err"] = ERR_EXIST;
            std::string jsonstr = root.toStyledString();
            beast::ostream(connection->res_.body()) << jsonstr;
            return true;
        }

        
        root["err"] = SUCCESS;
        root["uid"] = uid;
        root["email"] = email;
        root ["user"]= name;
        root["pwd"] = pwd;
        root["confirm"] = confirm;
        root["verifycode"] = verify_code;
        root["icon"] = icon;
        std::string jsonstr = root.toStyledString();
        beast::ostream(connection->res_.body()) << jsonstr;
        return true;

    });
    // 重置密码请求
    reg_posthandler("/reset_pwd", [](std::shared_ptr<http_conn> connection) {
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
        auto email = src_root["email"].asString();
        auto name = src_root["user"].asString();
        auto pwd = src_root["pwd"].asString();
        //先查找redis中email对应的验证码是否合理
        std::string  verify_code;
        bool b_get_varify = redis_mgr::get_instance()->get(CODEPREFIX + src_root["email"].asString(), verify_code);
        if (!b_get_varify) {
            std::cout << " get varify code expired" << std::endl;
            root["err"] = ERR_VER_CODE;
            std::string jsonstr = root.toStyledString();
            beast::ostream(connection->res_.body()) << jsonstr;
            return true;
        }
        if (verify_code != src_root["verify_code"].asString()) {
            std::cout << " verify code error" << std::endl;
            root["err"] = ERR_VER_CODE;
            std::string jsonstr = root.toStyledString();
            beast::ostream(connection->res_.body()) << jsonstr;
            return true;
        }
        //查询数据库判断用户名和邮箱是否匹配
        bool email_valid = mysql_mgr::get_instance()->check_email(name, email);
        if (!email_valid) {
            std::cout << " user email not match" << std::endl;
            root["err"] = ERR_EMAIL;
            std::string jsonstr = root.toStyledString();
            beast::ostream(connection->res_.body()) << jsonstr;
            return true;
        }
        //更新密码为最新密码
        bool b_up = mysql_mgr::get_instance()->update_pwd(name, pwd);
        if (!b_up) {
            std::cout << " update pwd failed" << std::endl;
            root["err"] = ERR_PWD_UPDATE;
            std::string jsonstr = root.toStyledString();
            beast::ostream(connection->res_.body()) << jsonstr;
            return true;
        }
        std::cout << "succeed to update password" << pwd << std::endl;
        root["err"] = 0;
        root["email"] = email;
        root["user"] = name;
        root["pwd"] = pwd;
        root["verify_code"] = verify_code;
        std::string jsonstr = root.toStyledString();
        beast::ostream(connection->res_.body()) << jsonstr;
        return true;
    });

    // 登入逻辑，发送登入请求，服务器接收到请求调用rpc服务区statue服务器查询chatserver
    // 返回一个chatserver地址和token，客户端发起tcp登入请求，连接到chatserver，chatserver区status查询token是否正确
    // 验证成功后返回
    reg_posthandler("/user_login", [](std::shared_ptr<http_conn> connection) {
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

		auto email = src_root["email"].asString();
		auto pwd = src_root["pwd"].asString();
		user_info u_info;
		//查询数据库判断用户名和密码是否匹配
		bool pwd_valid = mysql_mgr::get_instance()->check_pwd(email, pwd, u_info);
		if (!pwd_valid) {
			std::cout << " user or pwd not match" << std::endl;
			root["err"] = ERR_PWD_INVAILD;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->res_.body()) << jsonstr;
			return true;
		}

		// 使用rpc调用StatusServer上的服务找到合适的chatserver
		auto reply = status_grpc_client::get_instance()->GetChatServer(u_info.uid);
		if (reply.error()) {
			std::cout << " grpc get chat server failed, error is " << reply.error()<< std::endl;
			root["err"] = ERR_RPC;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->res_.body()) << jsonstr;
			return true;
		}

		std::cout << "succeed to load userinfo uid is " << u_info.uid << std::endl;
		root["err"] = SUCCESS;
		root["email"] = email;
		root["uid"] = u_info.uid;
		root["token"] = reply.token();
		root["host"] = reply.host();
		root["port"] = reply.port();
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