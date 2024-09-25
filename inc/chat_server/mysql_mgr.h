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
	bool check_pwd(const std::string& email, const std::string& pwd, user_info& userInfo);
	bool TestProcedure(const std::string &email, int& uid, std::string & name);
	std::shared_ptr<user_info> get_user(int uid);
	std::shared_ptr<user_info> get_user(std::string name);
	bool add_friend_apply(const int& from, const int& to);
	bool auth_friend_apply(const int& from, const int& to);
	bool add_friend(const int& from, const int& to, std::string back_name);
	bool get_apply_list(int touid, std::vector<std::shared_ptr<apply_info>>& applyList, int begin, int limit=10);

	bool get_friend_list(int self_id, std::vector<std::shared_ptr<user_info> >& user_info);
private:
	mysql_mgr();
	mysql_dao  _dao;
};


#endif
