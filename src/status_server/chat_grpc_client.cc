#include "chat_grpc_client.h"
#include "redis_mgr.h"

ChatGrpcClient::ChatGrpcClient()
{
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
		
		_pools[cfg[word]["name"]] = std::make_unique<ChatConPool>(5, cfg[word]["host"], cfg[word]["port"]);
	}

}

AddFriendRsp ChatGrpcClient::NotifyAddFriend(const AddFriendReq& req)
{
	auto to_uid = req.touid();
	std::string  uid_str = std::to_string(to_uid);
	
	AddFriendRsp rsp;
	return rsp;
}
