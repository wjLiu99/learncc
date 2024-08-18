#ifndef _MYSQL_MGR_H_
#define _MYSQL_MGR_H_
#include "comm.h"
#include "mysql_dao.h"
class MysqlMgr: public Singleton<MysqlMgr>
{
	friend class Singleton<MysqlMgr>;
public:
	~MysqlMgr();
	
	int RegUser(const std::string& name, const std::string& email,  const std::string& pwd);
	bool CheckEmail(const std::string& name, const std::string & email);
	bool UpdatePwd(const std::string& name, const std::string& email);
	bool CheckPwd(const std::string& email, const std::string& pwd, user_info& userInfo);
	bool TestProcedure(const std::string &email, int& uid, std::string & name);
	std::shared_ptr<user_info> GetUser(int uid);
	std::shared_ptr<user_info> GetUser(std::string name);
	bool add_friend_apply(const int& from, const int& to);
	bool auth_friend_apply(const int& from, const int& to);
	bool AddFriend(const int& from, const int& to, std::string back_name);
	bool GetApplyList(int touid, std::vector<std::shared_ptr<apply_info>>& applyList, int begin, int limit=10);

	bool get_friend_list(int self_id, std::vector<std::shared_ptr<user_info> >& user_info);
private:
	MysqlMgr();
	MysqlDao  _dao;
};


#endif
