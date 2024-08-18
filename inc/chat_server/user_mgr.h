#ifndef _USER_MGR_H_
#define _USER_MGR_H_
#include "singleton.h"
#include <unordered_map>
#include <memory>
#include <mutex>

// 用户管理类
class csession;
class user_mgr: public Singleton<user_mgr>
{
	friend class Singleton<user_mgr>;
public:
	~user_mgr();
	// 获取用户连接的session
	std::shared_ptr<csession> get_session(int uid);
	// 连接成功后设置用户session
	void set_user_session(int uid, std::shared_ptr<csession> session);
	// 退出登入后移除session
	void rm_user_session(int uid);
private:
	user_mgr();
	std::mutex session_mtx_;
	std::unordered_map<int, std::shared_ptr<csession>> uid_to_session_;
};
#endif