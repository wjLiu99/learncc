#ifndef _MYSQL_MGR_H_
#define _MYSQL_MGR_H_
#include "comm.h"
#include "mysql_dao.h"
class mysql_mgr: public Singleton<mysql_mgr>
{
	friend class Singleton<mysql_mgr>;
public:
	~mysql_mgr();
	// 注册用户
	int reg_user(const std::string& name, const std::string& email,  const std::string& pwd);
	// 检查邮箱是否正确
	bool check_email(const std::string& name, const std::string & email);
	// 更新密码
	bool update_pwd(const std::string& name, const std::string& email);
	// 检查密码，返回用户信息
	bool check_pwd(const std::string& email, const std::string& pwd, UserInfo& userInfo);
	
private:
	mysql_mgr();
	mysql_dao  _dao;
};

#endif