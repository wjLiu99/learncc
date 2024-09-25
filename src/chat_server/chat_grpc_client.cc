#include "chat_grpc_client.h"
#include "redis_mgr.h"
#include "conf_mgr.h"
#include "user_mgr.h"
#include "csession.h"
#include "mysql_mgr.h"

chat_grpc_client::chat_grpc_client()
{
	auto& cfg = conf_mgr::get_instance();
	auto server_list = cfg["peer_server"]["servers"];

	std::vector<std::string> words;

	std::stringstream ss(server_list);
	std::string word;

	while (std::getline(ss, word, ',')) {
		words.push_back(word);
	}

	for (auto& word : words) {
		if (cfg[word]["name"].empty()) {
			continue;
		}
        // 每一个对端创建一个连接池, 每一个chatserver既是rpc客户端也是rpc服务端
		pools_[cfg[word]["name"]] = std::make_unique<chat_grpc_conpool>(5, cfg[word]["host"], cfg[word]["port"]);
	}

}
// 通知添加好友
AddFriendRsp chat_grpc_client::NotifyAddFriend(std::string server_ip, const AddFriendReq& req)
{
	AddFriendRsp rsp;
	Defer defer([&rsp, &req]() {
		rsp.set_error(SUCCESS);
		rsp.set_applyuid(req.applyuid());
		rsp.set_touid(req.touid());
		});

	// 通过server ip找到rpc连接池
	auto find_iter = pools_.find(server_ip);
	if (find_iter == pools_.end()) {
		return rsp;
	}
	
	auto &pool = find_iter->second;
	ClientContext context;
	// 从rpc连接池获取一个连接
	auto stub = pool->get_conn();
	Status status = stub->NotifyAddFriend(&context, req, &rsp);
	Defer defercon([&stub, this, &pool]() {
		pool->return_conn(std::move(stub));
		});

	if (!status.ok()) {
		rsp.set_error(ERR_RPC);
		return rsp;
	}

	return rsp;
}


bool chat_grpc_client::get_base_info(std::string base_key, int uid, std::shared_ptr<user_info>& userinfo)
{
	//优先查redis中查询用户信息
	std::string info_str = "";
	bool b_base = redis_mgr::get_instance()->get(base_key, info_str);
	if (b_base) {
		Json::Reader reader;
		Json::Value root;
		reader.parse(info_str, root);
		userinfo->uid = root["uid"].asInt();
		userinfo->name = root["name"].asString();
		userinfo->pwd = root["pwd"].asString();
		userinfo->email = root["email"].asString();
		userinfo->nick = root["nick"].asString();
		userinfo->desc = root["desc"].asString();
		userinfo->sex = root["sex"].asInt();
		userinfo->icon = root["icon"].asString();
		std::cout << "user login uid is  " << userinfo->uid << " name  is "
			<< userinfo->name << " pwd is " << userinfo->pwd << " email is " << userinfo->email << endl;
	}
	else {
		//redis中没有则查询mysql
		//查询数据库
		std::shared_ptr<user_info> user_info = nullptr;
		user_info = mysql_mgr::get_instance()->get_user(uid);
		if (user_info == nullptr) {
			return false;
		}

		userinfo = user_info;

		//将数据库内容写入redis缓存
		Json::Value redis_root;
		redis_root["uid"] = uid;
		redis_root["pwd"] = userinfo->pwd;
		redis_root["name"] = userinfo->name;
		redis_root["email"] = userinfo->email;
		redis_root["nick"] = userinfo->nick;
		redis_root["desc"] = userinfo->desc;
		redis_root["sex"] = userinfo->sex;
		redis_root["icon"] = userinfo->icon;
		redis_mgr::get_instance()->set(base_key, redis_root.toStyledString());
	}

}

AuthFriendRsp chat_grpc_client::NotifyAuthFriend(std::string server_ip, const AuthFriendReq& req) {
	AuthFriendRsp rsp;
	rsp.set_error(SUCCESS);

	Defer defer([&rsp, &req]() {
		rsp.set_fromuid(req.fromuid());
		rsp.set_touid(req.touid());
		});

	auto find_iter = pools_.find(server_ip);
	if (find_iter == pools_.end()) {
		return rsp;
	}

	auto& pool = find_iter->second;
	ClientContext context;
	auto stub = pool->get_conn();
	Status status = stub->NotifyAuthFriend(&context, req, &rsp);
	Defer defercon([&stub, this, &pool]() {
		pool->return_conn(std::move(stub));
		});

	if (!status.ok()) {
		rsp.set_error(ERR_RPC);
		return rsp;
	}

	return rsp;
}

TextChatMsgRsp chat_grpc_client::NotifyTextChatMsg(std::string server_ip, 
	const TextChatMsgReq& req, const Json::Value& rtvalue) {
	
	TextChatMsgRsp rsp;
	rsp.set_error(SUCCESS);

	Defer defer([&rsp, &req]() {
		rsp.set_fromuid(req.fromuid());
		rsp.set_touid(req.touid());
		for (const auto& text_data : req.textmsgs()) {
			TextChatData* new_msg = rsp.add_textmsgs();
			new_msg->set_msgid(text_data.msgid());
			new_msg->set_msgcontent(text_data.msgcontent());
		}
		
		});

	auto find_iter = pools_.find(server_ip);
	if (find_iter == pools_.end()) {
		return rsp;
	}

	auto& pool = find_iter->second;
	ClientContext context;
	auto stub = pool->get_conn();
	Status status = stub->NotifyTextChatMsg(&context, req, &rsp);
	Defer defercon([&stub, this, &pool]() {
			pool->return_conn(std::move(stub));
		});

	if (!status.ok()) {
		rsp.set_error(ERR_RPC);
		return rsp;
	}

	return rsp;
}