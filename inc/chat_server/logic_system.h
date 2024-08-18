#pragma once
#include "csession.h"
#include "comm.h"
#include "data.h"
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>
// 消息处理回调函数
typedef  function<void(shared_ptr<csession>, const short &msg_id, const string &msg_data)> func_cb;

class logic_system:public Singleton<logic_system>
{
	friend class Singleton<logic_system>;
public:
	~logic_system();
	// 投递消息
	void post_msg(shared_ptr < logic_node> msg);
private:
	logic_system();
	// 线程函数
	void deal_msg();
	// 注册消息处理函数
	void reg_cbs();
	// 登入消息处理函数
	void login_handler(shared_ptr<csession>, const short &msg_id, const string &msg_data);
	// 搜索用户消息处理
	void search_info(std::shared_ptr<csession> session, const short& msg_id, const string& msg_data);
	// 添加好友申请消息处理
	void add_friend_apply(std::shared_ptr<csession> session, const short& msg_id, const string& msg_data);
	// 发送文本聊天消息处理
	void deal_chat_text_msg(std::shared_ptr<csession> session, const short& msg_id, const string& msg_data);
	// 获取好友列表
	bool get_friend_list(int self_id, std::vector<std::shared_ptr<user_info>>& user_list);
	// 认证好友申请消息处理
	void auth_friend_apply(std::shared_ptr<csession> session, const short& msg_id, const string& msg_data);
	// 判断是否是数字
	bool is_pure_digit(const std::string& str);
	// 获取用户信息并回写到redis
	void get_user_byuid(std::string uid_str, Json::Value& rtvalue);
	void get_user_byname(std::string name, Json::Value& rtvalue);
	bool get_base_info(std::string base_key, int uid, std::shared_ptr<user_info> &userinfo);
	// 获取好友申请列表
	bool get_friend_apply_info(int to_uid, std::vector<std::shared_ptr<apply_info>>& list);

	std::thread worker_thread_;
	std::queue<shared_ptr<logic_node>> msg_que_;
	std::mutex mtx_;
	std::condition_variable consume_;
	bool b_stop_;
	std::map<short, func_cb> fun_callbacks_;
	std::unordered_map<int, std::shared_ptr<user_info>> users_;

};
