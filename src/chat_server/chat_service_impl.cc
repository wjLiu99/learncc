#include "chat_service_impl.h"
#include "user_mgr.h"
#include "csession.h"
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>
#include "redis_mgr.h"
#include "mysql_mgr.h"
#include "user_mgr.h"
#include "comm.h"
ChatServiceImpl::ChatServiceImpl()
{

}
// 收到通知添加好友服务请求
Status ChatServiceImpl::NotifyAddFriend(ServerContext* context, const AddFriendReq* request, AddFriendRsp* reply)
{
	//查找用户是否在本服务器
	auto touid = request->touid();
	auto session = user_mgr::get_instance()->get_session(touid);

	Defer defer([request, reply]() {
		reply->set_error(SUCCESS);
		reply->set_applyuid(request->applyuid());
		reply->set_touid(request->touid());
		});

	//用户不在内存中则直接返回
	if (session == nullptr) {
		return Status::OK;
	}
	
	//在内存中则直接发送通知对方
	Json::Value  rtvalue;
	rtvalue["err"] = SUCCESS;
	rtvalue["applyuid"] = request->applyuid();
	rtvalue["name"] = request->name();
	rtvalue["desc"] = request->desc();
	rtvalue["icon"] = request->icon();
	rtvalue["sex"] = request->sex();
	rtvalue["nick"] = request->nick();

	std::string return_str = rtvalue.toStyledString();

	session->send(return_str, ID_NOTIFY_ADD_FRIEND_REQ);
	return Status::OK;
}
// 收到添加好友认证消息，由对端服务器调用rpc服务通知该服务器，处理认证消息
Status ChatServiceImpl::NotifyAuthFriend(ServerContext* context, const AuthFriendReq* request,
	AuthFriendRsp* reply) {
	//查找用户是否在本服务器
	auto touid = request->touid();
	auto fromuid = request->fromuid();
	auto session = user_mgr::get_instance()->get_session(touid);

	Defer defer([request, reply]() {
		reply->set_error(SUCCESS);
		reply->set_fromuid(request->fromuid());
		reply->set_touid(request->touid());
		});

	//用户不在内存中则直接返回
	if (session == nullptr) {
		return Status::OK;
	}

	//在内存中则直接发送通知对方
	Json::Value  rtvalue;
	rtvalue["err"] = SUCCESS;
	rtvalue["fromuid"] = request->fromuid();
	rtvalue["touid"] = request->touid();

	std::string base_key = USER_BASE_INFO + std::to_string(fromuid);
	auto u_info = std::make_shared<user_info>();
	bool b_info = get_base_info(base_key, fromuid, u_info);
	if (b_info) {
		rtvalue["name"] = u_info->name;
		rtvalue["nick"] = u_info->nick;
		rtvalue["icon"] = u_info->icon;
		rtvalue["sex"] = u_info->sex;
	}
	else {
		rtvalue["err"] = ERR_UID;
	}

	std::string return_str = rtvalue.toStyledString();

	session->send(return_str, ID_NOTIFY_AUTH_FRIEND_REQ);
	return Status::OK;
}

Status ChatServiceImpl::NotifyTextChatMsg(::grpc::ServerContext* context,
	const TextChatMsgReq* request, TextChatMsgRsp* reply) {
	//查找用户是否在本服务器
	auto touid = request->touid();
	auto session = user_mgr::get_instance()->get_session(touid);
	reply->set_error(SUCCESS);

	//用户不在内存中则直接返回
	if (session == nullptr) {
		return Status::OK;
	}

	//在内存中则直接发送通知对方
	Json::Value  rtvalue;
	rtvalue["err"] = SUCCESS;
	rtvalue["fromuid"] = request->fromuid();
	rtvalue["touid"] = request->touid();

	//将聊天数据组织为数组
	Json::Value text_array;
	for (auto& msg : request->textmsgs()) {
		Json::Value element;
		element["content"] = msg.msgcontent();
		element["msgid"] = msg.msgid();
		text_array.append(element);
	}
	rtvalue["text_array"] = text_array;

	std::string return_str = rtvalue.toStyledString();

	session->send(return_str, ID_NOTIFY_TEXT_CHAT_MSG_REQ);
	return Status::OK;
}


bool ChatServiceImpl::get_base_info(std::string base_key, int uid, std::shared_ptr<user_info>& userinfo)
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
		user_info = MysqlMgr::get_instance()->GetUser(uid);
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