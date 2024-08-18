#include "user_mgr.h"
#include "csession.h"
#include "redis_mgr.h"

user_mgr:: ~user_mgr() {
	uid_to_session_.clear();
} 


std::shared_ptr<csession> user_mgr::get_session(int uid)
{
	std::lock_guard<std::mutex> lock(session_mtx_);
	auto iter = uid_to_session_.find(uid);
	if (iter == uid_to_session_.end()) {
		return nullptr;
	}

	return iter->second;
}

void user_mgr::set_user_session(int uid, std::shared_ptr<csession> session)
{
	std::lock_guard<std::mutex> lock(session_mtx_);
	uid_to_session_[uid] = session;
}

void user_mgr::rm_user_session(int uid)
{ 
	auto uid_str = std::to_string(uid);
	// 因为再次登录可能是其他服务器，所以会造成本服务器删除key，其他服务器注册key的情况
	// 有可能其他服务登录，本服删除key造成找不到key的情况
	// 线程不安全，可能其他服务器已经注册了，本服务器再删除就可能导致误删
	//RedisMgr::GetInstance()->Del(USERIPPREFIX + uid_str);

	{
		std::lock_guard<std::mutex> lock(session_mtx_);
		uid_to_session_.erase(uid);
	}

}

user_mgr::user_mgr()
{

}
