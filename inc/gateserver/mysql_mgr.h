#ifndef _MYSQL_MGR_H_
#define _MYSQL_MGR_H_
#include "comm.h"
#include "mysql_dao.h"
class mysql_mgr: public Singleton<mysql_mgr>
{
	friend class Singleton<mysql_mgr>;
public:
	~mysql_mgr();
	int reg_user(const std::string& name, const std::string& email,  const std::string& pwd);
	bool check_email(const std::string& name, const std::string & email);
	bool update_pwd(const std::string& name, const std::string& email);
	bool check_pwd(const std::string& email, const std::string& pwd, user_info& u_info);
	
private:
	mysql_mgr();
	mysql_dao  _dao;
};

#endif
