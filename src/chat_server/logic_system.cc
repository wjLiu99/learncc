#include "logic_system.h"
#include "status_grpc_client.h"
#include "mysql_mgr.h"
#include "redis_mgr.h"
#include "user_mgr.h"
#include "chat_grpc_client.h"

using namespace std;

logic_system::logic_system():b_stop_(false){
	reg_cbs();
	worker_thread_ = std::thread (&logic_system::deal_msg, this);
}

logic_system::~logic_system(){
	{
		std::unique_lock<std::mutex> unique_lk(mtx_);
		b_stop_ = true;
		consume_.notify_one();
	}
	worker_thread_.join();
}

// 向消息队列中投递消息
void logic_system::post_msg(shared_ptr < logic_node> msg) {
	std::unique_lock<std::mutex> unique_lk(mtx_);
	msg_que_.push(msg);
	//由0变为1则发送通知信号
	if (msg_que_.size() == 1) {
		unique_lk.unlock();
		consume_.notify_one();
	}
}
// LogicSystem线程函数，从消息队列中取出消息根据消息id找到相应的回调函数处理
void logic_system::deal_msg() {
	for (;;) {
		std::unique_lock<std::mutex> unique_lk(mtx_);
		//判断队列为空则用条件变量阻塞等待，并释放锁
		while (msg_que_.empty() && !b_stop_) {
			consume_.wait(unique_lk);
		}

		//判断是否为关闭状态，把所有逻辑执行完后则退出循环
		if (b_stop_) {
			while (!msg_que_.empty()) {
				auto msg_node = msg_que_.front();
				cout << "recv_msg id  is " << msg_node->recv_node_->_msg_id << endl;
				auto call_back_iter = fun_callbacks_.find(msg_node->recv_node_->_msg_id);
				if (call_back_iter == fun_callbacks_.end()) {
					msg_que_.pop();
					continue;
				}
				call_back_iter->second(msg_node->session_, msg_node->recv_node_->_msg_id,
					std::string(msg_node->recv_node_->data_, msg_node->recv_node_->_cur_len));
				msg_que_.pop();
			}
			break;
		}

		//如果没有停服，且说明队列中有数据
		auto msg_node = msg_que_.front();
		cout << "recv_msg id  is " << msg_node->recv_node_->_msg_id << endl;
		auto call_back_iter = fun_callbacks_.find(msg_node->recv_node_->_msg_id);
		if (call_back_iter == fun_callbacks_.end()) {
			msg_que_.pop();
			std::cout << "msg id [" << msg_node->recv_node_->_msg_id << "] handler not found" << std::endl;
			continue;
		}
		call_back_iter->second(msg_node->session_, msg_node->recv_node_->_msg_id, 
			std::string(msg_node->recv_node_->data_, msg_node->recv_node_->_cur_len));
		msg_que_.pop();
	}
}

// 注册回调函数
void logic_system::reg_cbs() {
	fun_callbacks_[MSG_CHAT_LOGIN] = std::bind(&logic_system::login_handler, this,
		placeholders::_1, placeholders::_2, placeholders::_3);

	fun_callbacks_[ID_SEARCH_USER_REQ] = std::bind(&logic_system::login_handler, this,
		placeholders::_1, placeholders::_2, placeholders::_3);

	fun_callbacks_[ID_ADD_FRIEND_REQ] = std::bind(&logic_system::add_friend_apply, this,
		placeholders::_1, placeholders::_2, placeholders::_3);

	fun_callbacks_[ID_AUTH_FRIEND_REQ] = std::bind(&logic_system::auth_friend_apply, this,
		placeholders::_1, placeholders::_2, placeholders::_3);

	fun_callbacks_[ID_TEXT_CHAT_MSG_REQ] = std::bind(&logic_system::deal_chat_text_msg, this,
		placeholders::_1, placeholders::_2, placeholders::_3);
}

// 登入请求处理函数
void logic_system::login_handler(shared_ptr<csession> session, const short &msg_id, const string &msg_data) {
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);
	auto uid = root["uid"].asInt();
	auto token = root["token"].asString();
	std::cout << "user login uid is  " << uid << " user token  is "
		<< root["token"].asString() << endl;


	Json::Value  rtvalue;
	// 发送回包
	Defer defer([this, &rtvalue, session]() {
		std::string return_str = rtvalue.toStyledString();
		session->send(return_str, MSG_CHAT_LOGIN_RSP);
		});

	//从redis获取用户token是否正确
	std::string uid_str = std::to_string(uid);
	std::string token_key = USERTOKENPREFIX + uid_str;
	std::string token_value = "";
	bool success = redis_mgr::get_instance()->get(token_key, token_value);
	if (!success) {
		rtvalue["err"] = ERR_UID;
		return ;
	}

	if (token_value != token) {
		rtvalue["err"] = ERR_TOKEN;
		return ;
	}

	rtvalue["err"] = SUCCESS;

	// redis中查询用户信息
	std::string base_key = USER_BASE_INFO + uid_str;
	auto u_info = std::make_shared<user_info>();
	bool b_base = get_base_info(base_key, uid, u_info);
	if (!b_base) {
		rtvalue["err"] = ERR_UID;
		return;
	}
	rtvalue["uid"] = uid;
	rtvalue["pwd"] = u_info->pwd;
	rtvalue["name"] = u_info->name;
	rtvalue["email"] = u_info->email;
	rtvalue["nick"] = u_info->nick;
	rtvalue["desc"] = u_info->desc;
	rtvalue["sex"] = u_info->sex;

	//从数据库获取申请列表, 好友申请消息需要存在数据库，可以添加离线好友，登入时加载好友申请请求
	std::vector<std::shared_ptr<apply_info>> apply_list;
	auto b_apply = get_friend_apply_info(uid,apply_list);
	if (b_apply) {
		for (auto & apply : apply_list) {
			Json::Value obj;
			obj["name"] = apply->name_;
			obj["uid"] = apply->uid_;
			obj["icon"] = apply->icon_;
			obj["nick"] = apply->nick_;
			obj["sex"] = apply->sex_;
			obj["desc"] = apply->desc_;
			obj["status"] = apply->status_;
			rtvalue["apply_list"].append(obj);
		}
	}

	//获取好友列表
	std::vector<std::shared_ptr<user_info>> friend_list;
	bool b_friend_list = get_friend_list(uid, friend_list);
	for (auto& friend_ele : friend_list) {
		Json::Value obj;
		obj["name"] = friend_ele->name;
		obj["uid"] = friend_ele->uid;
		obj["icon"] = friend_ele->icon;
		obj["nick"] = friend_ele->nick;
		obj["sex"] = friend_ele->sex;
		obj["desc"] = friend_ele->desc;
		obj["back"] = friend_ele->back;
		rtvalue["friend_list"].append(obj);
	}


	auto server_name = conf_mgr::get_instance().get_value("self_server", "Name");
	//将登录数量增加
	auto rd_res = redis_mgr::get_instance()->hget(LOGIN_COUNT, server_name);
	int count = 0;
	if (!rd_res.empty()) {
		count = std::stoi(rd_res);
	}

	count++;
	auto count_str = std::to_string(count);
	redis_mgr::get_instance()->hset(LOGIN_COUNT, server_name, count_str);
	//session绑定用户uid
	session->set_user_id(uid);
	//为用户设置登录ip server的名字
	std::string  ipkey = USERIPPREFIX + uid_str;
	redis_mgr::get_instance()->set(ipkey, server_name);
	//uid和session绑定管理,方便以后踢人操作
	user_mgr::get_instance()->set_user_session(uid, session);

	return;
}

//从mysql获取好友列表
bool logic_system::get_friend_list(int self_id, std::vector<std::shared_ptr<user_info>>& user_list) {
	
	return MysqlMgr::get_instance()->get_friend_list(self_id, user_list);
}

// 查询用户信息 优先查redis 没有查到就去mysql查询并回写到redis
bool logic_system::get_base_info(std::string base_key, int uid, std::shared_ptr<user_info>& userinfo)
{
	
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
		std::cout << "user login uid is  " << userinfo->uid << " name  is "
			<< userinfo->name << " pwd is " << userinfo->pwd << " email is " << userinfo->email << endl;
	}
	else {
		//redis中没有则查询mysql
		//查询数据库
		std::shared_ptr<user_info> u_info = nullptr;
		u_info = MysqlMgr::get_instance()->GetUser(uid);
		if (u_info == nullptr) {
			return false;
		}

		userinfo = u_info;

		//将数据库内容写入redis缓存
		Json::Value redis_root;
		redis_root["uid"] = uid;
		redis_root["pwd"] = userinfo->pwd;
		redis_root["name"] = userinfo->name;
		redis_root["email"] = userinfo->email;
		redis_root["nick"] = userinfo->nick;
		redis_root["desc"] = userinfo->desc;
		redis_root["sex"] = userinfo->sex;

		redis_mgr::get_instance()->set(base_key, redis_root.toStyledString());
	}
	return true;

}

//从mysql获取好友申请列表
bool logic_system::get_friend_apply_info(int to_uid, std::vector<std::shared_ptr<apply_info>> &list) {
	
	return MysqlMgr::get_instance()->GetApplyList(to_uid, list, 0, 10);
}


// 认证好友申请处理函数
void logic_system::auth_friend_apply(std::shared_ptr<csession> session, const short& msg_id, const string& msg_data) {
	
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);

	auto uid = root["fromuid"].asInt();
	auto touid = root["touid"].asInt();
	auto back_name = root["back"].asString();
	std::cout << "from " << uid << " auth friend to " << touid << std::endl;

	Json::Value  rtvalue;
	rtvalue["err"] = SUCCESS;
	auto u_info = std::make_shared<user_info>();

	std::string base_key = USER_BASE_INFO + std::to_string(touid);
	bool b_info = get_base_info(base_key, touid, u_info);
	if (b_info) {
		rtvalue["name"] = u_info->name;
		rtvalue["nick"] = u_info->nick;
		rtvalue["icon"] = u_info->icon;
		rtvalue["sex"] = u_info->sex;
		rtvalue["uid"] = touid;
	}
	else {
		rtvalue["err"] = ERR_UID;
	}


	Defer defer([this, &rtvalue, session]() {
		std::string return_str = rtvalue.toStyledString();
		session->send(return_str, ID_AUTH_FRIEND_RSP);
		});

	//先更新数据库
	MysqlMgr::get_instance()->auth_friend_apply(uid, touid);

	//更新数据库添加好友
	MysqlMgr::get_instance()->AddFriend(uid, touid, back_name);

	//查询redis 查找touid对应的server ip
	auto to_str = std::to_string(touid);
	auto to_ip_key = USERIPPREFIX + to_str;
	std::string to_ip_value = "";
	bool b_ip = redis_mgr::get_instance()->get(to_ip_key, to_ip_value);
	if (!b_ip) {
		return;
	}

	auto& cfg = conf_mgr::get_instance();
	auto self_name = cfg["self_server"]["name"];
	// 直接通知对方有认证通过消息
	if (to_ip_value == self_name) {
		auto session = user_mgr::get_instance()->get_session(touid);
		if (session) {
			//在内存中则直接发送通知对方
			Json::Value  notify;
			notify["err"] = SUCCESS;
			notify["fromuid"] = uid;
			notify["touid"] = touid;
			std::string base_key = USER_BASE_INFO + std::to_string(uid);
			auto u_info = std::make_shared<user_info>();
			bool b_info = get_base_info(base_key, uid, u_info);
			if (b_info) {
				notify["name"] = u_info->name;
				notify["nick"] = u_info->nick;
				notify["icon"] = u_info->icon;
				notify["sex"] = u_info->sex;
			}
			else {
				notify["err"] = ERR_UID;
			}


			std::string return_str = notify.toStyledString();
			session->send(return_str, ID_NOTIFY_AUTH_FRIEND_REQ);
		}

		return ;
	}

	//	session不在本机，远程过程调用
	AuthFriendReq auth_req;
	auth_req.set_fromuid(uid);
	auth_req.set_touid(touid);

	//发送通知
	chat_grpc_client::get_instance()->NotifyAuthFriend(to_ip_value, auth_req);
}

// 处理搜索请求
void logic_system::search_info(std::shared_ptr<csession> session, const short& msg_id, const string& msg_data)
{
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);
	auto uid_str = root["uid"].asString();
	std::cout << "user login uid is  " << uid_str << endl;

	Json::Value  rtvalue;
	// 回复消息
	Defer defer([this, &rtvalue, session]() {
		std::string return_str = rtvalue.toStyledString();
		session->send(return_str, ID_SEARCH_USER_RSP);
		});

	bool b_digit = is_pure_digit(uid_str);
	if (b_digit) {
		get_user_byuid(uid_str, rtvalue);
	}
	else {
		get_user_byname(uid_str, rtvalue);
	}
	return;
}


bool logic_system::is_pure_digit(const std::string& str)
{
	for (char c : str) {
		if (!std::isdigit(c)) {
			return false;
		}
	}
	return true;
}

// 处理文本聊天信息
void logic_system::deal_chat_text_msg(std::shared_ptr<csession> session, const short& msg_id, const string& msg_data) {
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);

	auto uid = root["fromuid"].asInt();
	auto touid = root["touid"].asInt();

	const Json::Value  arrays = root["text_array"];
	
	Json::Value  rtvalue;
	rtvalue["err"] = SUCCESS;
	rtvalue["text_array"] = arrays;
	rtvalue["fromuid"] = uid;
	rtvalue["touid"] = touid;
	// 发送回包
	Defer defer([this, &rtvalue, session]() {
		std::string return_str = rtvalue.toStyledString();
		session->send(return_str, ID_TEXT_CHAT_MSG_RSP);
		});


	//查询redis 查找touid对应的server ip
	auto to_str = std::to_string(touid);
	auto to_ip_key = USERIPPREFIX + to_str;
	std::string to_ip_value = "";
	bool b_ip = redis_mgr::get_instance()->get(to_ip_key, to_ip_value);
	if (!b_ip) {
		return;
	}

	auto& cfg = conf_mgr::get_instance();
	auto self_name = cfg["self_server"]["name"];
	//直接通知对方有认证通过消息
	if (to_ip_value == self_name) {
		auto session = user_mgr::get_instance()->get_session(touid);
		if (session) {
			//在内存中则直接发送通知对方
			std::string return_str = rtvalue.toStyledString();
			session->send(return_str, ID_NOTIFY_TEXT_CHAT_MSG_REQ);
		}

		return ;
	}

	// 对方session不在本机，同rpc调用传递信息
	TextChatMsgReq text_msg_req;
	text_msg_req.set_fromuid(uid);
	text_msg_req.set_touid(touid);
	for (const auto& txt_obj : arrays) {
		auto content = txt_obj["content"].asString();
		auto msgid = txt_obj["msgid"].asString();
		std::cout << "content is " << content << std::endl;
		std::cout << "msgid is " << msgid << std::endl;
		auto *text_msg = text_msg_req.add_textmsgs();
		text_msg->set_msgid(msgid);
		text_msg->set_msgcontent(content);
	}


	//发送通知 todo...
	chat_grpc_client::get_instance()->NotifyTextChatMsg(to_ip_value, text_msg_req, rtvalue);
}

// 通过用户uid查询用户信息
void logic_system::get_user_byuid(std::string uid_str, Json::Value& rtvalue)
{
	rtvalue["err"] = SUCCESS;

	std::string base_key = USER_BASE_INFO + uid_str;

	//优先查redis中查询用户信息
	std::string info_str = "";
	bool b_base = redis_mgr::get_instance()->get(base_key, info_str);
	if (b_base) {
		Json::Reader reader;
		Json::Value root;
		reader.parse(info_str, root);
		auto uid = root["uid"].asInt();
		auto name = root["name"].asString();
		auto pwd = root["pwd"].asString();
		auto email = root["email"].asString();
		auto nick = root["nick"].asString();
		auto desc = root["desc"].asString();
		auto sex = root["sex"].asInt();
		std::cout << "user  uid is  " << uid << " name  is "
			<< name << " pwd is " << pwd << " email is " << email << endl;

		rtvalue["uid"] = uid;
		rtvalue["pwd"] = pwd;
		rtvalue["name"] = name;
		rtvalue["email"] = email;
		rtvalue["nick"] = nick;
		rtvalue["desc"] = desc;
		rtvalue["sex"] = sex;
		return;
	}

	auto uid = std::stoi(uid_str);
	//redis中没有则查询mysql
	//查询数据库
	std::shared_ptr<user_info> u_info = nullptr;
	u_info = MysqlMgr::get_instance()->GetUser(uid);
	if (u_info == nullptr) {
		rtvalue["err"] = ERR_UID;
		return;
	}

	//将数据库内容写入redis缓存
	Json::Value redis_root;
	redis_root["uid"] = u_info->uid;
	redis_root["pwd"] = u_info->pwd;
	redis_root["name"] = u_info->name;
	redis_root["email"] = u_info->email;
	redis_root["nick"] = u_info->nick;
	redis_root["desc"] = u_info->desc;
	redis_root["sex"] = u_info->sex;

	redis_mgr::get_instance()->set(base_key, redis_root.toStyledString());

	// auto server_name = conf_mgr::get_instance().get_value("self_server", "Name");
	// //将登录数量增加
	// auto rd_res = redis_mgr::get_instance()->hget(LOGIN_COUNT, server_name);
	// int count = 0;
	// if (!rd_res.empty()) {
	// 	count = std::stoi(rd_res);
	// }

	// count++;
	// auto count_str = std::to_string(count);
	// redis_mgr::get_instance()->hset(LOGIN_COUNT, server_name, count_str);

	//返回数据
	rtvalue["uid"] = u_info->uid;
	rtvalue["pwd"] = u_info->pwd;
	rtvalue["name"] = u_info->name;
	rtvalue["email"] = u_info->email;
	rtvalue["nick"] = u_info->nick;
	rtvalue["desc"] = u_info->desc;
	rtvalue["sex"] = u_info->sex;
}

// 通过用户的name字段查询用户信息
void logic_system::get_user_byname(std::string name, Json::Value& rtvalue)
{
	rtvalue["err"] = SUCCESS;

	std::string base_key = NAME_INFO + name;

	//优先查redis中查询用户信息
	std::string info_str = "";
	bool b_base = redis_mgr::get_instance()->get(base_key, info_str);
	if (b_base) {
		Json::Reader reader;
		Json::Value root;
		reader.parse(info_str, root);
		auto uid = root["uid"].asInt();
		auto name = root["name"].asString();
		auto pwd = root["pwd"].asString();
		auto email = root["email"].asString();
		auto nick = root["nick"].asString();
		auto desc = root["desc"].asString();
		auto sex = root["sex"].asInt();
		std::cout << "user  uid is  " << uid << " name  is "
			<< name << " pwd is " << pwd << " email is " << email << endl;

		rtvalue["uid"] = uid;
		rtvalue["pwd"] = pwd;
		rtvalue["name"] = name;
		rtvalue["email"] = email;
		rtvalue["nick"] = nick;
		rtvalue["desc"] = desc;
		rtvalue["sex"] = sex;
		return;
	}

	//redis中没有则查询mysql
	//查询数据库
	std::shared_ptr<user_info> u_info = nullptr;
	u_info = MysqlMgr::get_instance()->GetUser(name);
	if (u_info == nullptr) {
		rtvalue["err"] = ERR_UID;
		return;
	}

	//将数据库内容写入redis缓存
	Json::Value redis_root;
	redis_root["uid"] = u_info->uid;
	redis_root["pwd"] = u_info->pwd;
	redis_root["name"] = u_info->name;
	redis_root["email"] = u_info->email;
	redis_root["nick"] = u_info->nick;
	redis_root["desc"] = u_info->desc;
	redis_root["sex"] = u_info->sex;

	redis_mgr::get_instance()->set(base_key, redis_root.toStyledString());

	//返回数据
	rtvalue["uid"] = u_info->uid;
	rtvalue["pwd"] = u_info->pwd;
	rtvalue["name"] = u_info->name;
	rtvalue["email"] = u_info->email;
	rtvalue["nick"] = u_info->nick;
	rtvalue["desc"] = u_info->desc;
	rtvalue["sex"] = u_info->sex;
}




// 添加好友申请处理函数
void logic_system::add_friend_apply(std::shared_ptr<csession> session, const short& msg_id, const string& msg_data)
{
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);
	auto uid = root["uid"].asInt();
	auto applyname = root["applyname"].asString();
	auto bakname = root["bakname"].asString();
	auto touid = root["touid"].asInt();
	std::cout << "user login uid is  " << uid << " applyname  is "
		<< applyname << " bakname is " << bakname << " touid is " << touid << endl;

	Json::Value  rtvalue;
	rtvalue["err"] = SUCCESS;
	Defer defer([this, &rtvalue, session]() {
		std::string return_str = rtvalue.toStyledString();
		session->send(return_str, ID_ADD_FRIEND_RSP);
		});

	//先更新数据库
	MysqlMgr::get_instance()->add_friend_apply(uid, touid);

	//查询redis 查找touid对应的server ip
	auto to_str = std::to_string(touid);
	auto to_ip_key = USERIPPREFIX + to_str;
	std::string to_ip_value = "";
	bool b_ip = redis_mgr::get_instance()->get(to_ip_key, to_ip_value);
	if (!b_ip) {
		return;
	}
	// 如果是本机，就直接处理
	
	auto& cfg = conf_mgr::get_instance();
	auto self_name = cfg["self_server"]["name"];
	//直接通知对方有申请消息
	if (to_ip_value == self_name) {
		// 获取对端session
		auto session = user_mgr::get_instance()->get_session(touid);
		if (session) {
			//在内存中则直接发送通知对方
			Json::Value  notify;
			notify["err"] = SUCCESS;
			notify["applyuid"] = uid;
			notify["name"] = applyname;
			notify["desc"] = "";
			std::string return_str = notify.toStyledString();
			session->send(return_str, ID_NOTIFY_ADD_FRIEND_REQ);
		}

		return ;
	}

	// 对端session不在本服务器上，通过rpc调用到对端所在的服务器上
	std::string base_key = USER_BASE_INFO + std::to_string(uid);
	auto apply_info = std::make_shared<user_info>();
	bool b_info = get_base_info(base_key, uid, apply_info);
	

	AddFriendReq add_req;
	add_req.set_applyuid(uid);
	add_req.set_touid(touid);
	add_req.set_name(applyname);
	add_req.set_desc("");
	if (b_info) {
		add_req.set_icon(apply_info->icon);
		add_req.set_sex(apply_info->sex);
		add_req.set_nick(apply_info->nick);
	}

	// 发送通知
	chat_grpc_client::get_instance()->NotifyAddFriend(to_ip_value,add_req);

}