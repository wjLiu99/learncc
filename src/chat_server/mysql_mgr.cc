#include "mysql_mgr.h"
using namespace std;

MysqlMgr::~MysqlMgr() {

}

int MysqlMgr::RegUser(const std::string& name, const std::string& email, const std::string& pwd)
{
	return _dao.RegUser(name, email, pwd);
}

bool MysqlMgr::CheckEmail(const std::string& name, const std::string& email) {
	return _dao.CheckEmail(name, email);
}

bool MysqlMgr::UpdatePwd(const std::string& name, const std::string& pwd) {
	return _dao.UpdatePwd(name, pwd);
}

MysqlMgr::MysqlMgr() {
}

bool MysqlMgr::CheckPwd(const std::string& email, const std::string& pwd, user_info& userInfo) {
	return _dao.CheckPwd(email, pwd, userInfo);
}

bool MysqlMgr::TestProcedure(const std::string& email, int& uid, string& name) {
	return _dao.TestProcedure(email,uid, name);
}

std::shared_ptr<user_info> MysqlMgr::GetUser(int uid) {
	return _dao.GetUser(uid);
}

std::shared_ptr<user_info> MysqlMgr::GetUser(std::string name)
{
	return _dao.GetUser(name);
}

bool MysqlMgr::GetApplyList(int touid, 
	std::vector<std::shared_ptr<apply_info>>& applyList, int begin, int limit) {

	return _dao.GetApplyList(touid, applyList, begin, limit);
}

bool MysqlMgr::add_friend_apply(const int& from, const int& to)
{
	return _dao.add_friend_apply(from, to);
}

bool MysqlMgr::auth_friend_apply(const int& from, const int& to) {
	return _dao.auth_friend_apply(from, to);
}

bool MysqlMgr::AddFriend(const int& from, const int& to, std::string back_name) {
	return _dao.AddFriend(from, to, back_name);
}

bool MysqlMgr::get_friend_list(int self_id, std::vector<std::shared_ptr<user_info> >& user_info) {
	return _dao.get_friend_list(self_id, user_info);
}




