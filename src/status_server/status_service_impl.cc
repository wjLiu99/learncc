#include "status_service_impl.h"
#include "conf_mgr.h"
#include "comm.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include "redis_mgr.h"

// 生成唯一的字符串
std::string generate_unique_string() {
	// 创建UUID对象
	boost::uuids::uuid uuid = boost::uuids::random_generator()();
	// 将UUID转换为字符串
	std::string unique_string = to_string(uuid);

	return unique_string;
}
// 获取chatserver地址和登入token， 客户端登入时先访问gateserver，由gateserver调用该服务
Status status_service_impl::GetChatServer(ServerContext* context, const GetChatServerReq* request, GetChatServerRsp* reply)
{
	std::string prefix("status server has received :  ");
	const auto& server = get_chatserver();
	reply->set_host(server.host);
	reply->set_port(server.port);
	reply->set_error(SUCCESS);
	// 生成一个随机的token用于登入验证
	reply->set_token(generate_unique_string());
	// 向redis插入token，login时去查redis验证token是否正确
	insert_token(request->uid(), reply->token());
	return Status::OK;
}

status_service_impl::status_service_impl()
{
	// 添加服务器信息
	auto& cfg = conf_mgr::get_instance();
	auto server_list = cfg["chatservers"]["name"];

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

		chat_server server;
		server.port = cfg[word]["port"];
		server.host = cfg[word]["host"];
		server.name = cfg[word]["name"];
		servers_[server.name] = server;
	}
}

chat_server status_service_impl::get_chatserver() {

	std::lock_guard<std::mutex> guard(server_mtx_);
	// 登入数量最少的chatserver
	auto min_server = servers_.begin()->second;
	// 在redis中查询服务器在线人数
	auto count_str = redis_mgr::get_instance()->hget(LOGIN_COUNT, min_server.name);
	if (count_str.empty()) {
		// 不存在则默认设置为最大，说明服务器不在线
		min_server.con_count = INT_MAX;
	}
	else {
		min_server.con_count = std::stoi(count_str);
	}


	// 遍历server列表找到登入数量最少的服务器
	for ( auto& server : servers_) {
		
		if (server.second.name == min_server.name) {
			continue;
		}

		auto count_str = redis_mgr::get_instance()->hget(LOGIN_COUNT, server.second.name);
		if (count_str.empty()) {
			server.second.con_count = INT_MAX;
		}
		else {
			server.second.con_count = std::stoi(count_str);
		}

		if (server.second.con_count < min_server.con_count) {
			min_server = server.second;
		}
	}

	return min_server;
}

// login服务， 由chatserver调用， 验证token是否正确
Status status_service_impl::Login(ServerContext* context, const LoginReq* request, LoginRsp* reply)
{
	auto uid = request->uid();
	auto token = request->token();

	std::string uid_str = std::to_string(uid);
	std::string token_key = USERTOKENPREFIX + uid_str;
	std::string token_value = "";
	bool success = redis_mgr::get_instance()->get(token_key, token_value);
	if (success) {
		reply->set_error(ERR_UID);
		return Status::OK;
	}
	
	if (token_value != token) {
		reply->set_error(ERR_TOKEN);
		return Status::OK;
	}
	reply->set_error(SUCCESS);
	reply->set_uid(uid);
	reply->set_token(token);
	return Status::OK;
}

void status_service_impl::insert_token(int uid, std::string token)
{
	std::string uid_str = std::to_string(uid);
	std::string token_key = USERTOKENPREFIX + uid_str;
	redis_mgr::get_instance()->set(token_key, token);
}

