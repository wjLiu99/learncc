#include "mysql_mgr.h"
using namespace std;

mysql_mgr::~mysql_mgr() {

}

int mysql_mgr::reg_user(const std::string& name, const std::string& email, const std::string& pwd)
{
	return _dao.reg_user(name, email, pwd);
}

bool mysql_mgr::check_email(const std::string& name, const std::string& email) {
	return _dao.check_email(name, email);
}

bool mysql_mgr::update_pwd(const std::string& name, const std::string& pwd) {
	return _dao.update_pwd(name, pwd);
}

mysql_mgr::mysql_mgr() {
}

bool mysql_mgr::check_pwd(const std::string& email, const std::string& pwd, user_info& userInfo) {
	return _dao.check_pwd(email, pwd, userInfo);
}

bool mysql_mgr::TestProcedure(const std::string& email, int& uid, string& name) {
	return _dao.TestProcedure(email,uid, name);
}

std::shared_ptr<user_info> mysql_mgr::get_user(int uid) {
	return _dao.get_user(uid);
}

std::shared_ptr<user_info> mysql_mgr::get_user(std::string name)
{
	return _dao.get_user(name);
}

bool mysql_mgr::get_apply_list(int touid, 
	std::vector<std::shared_ptr<apply_info>>& applyList, int begin, int limit) {

	return _dao.get_apply_list(touid, applyList, begin, limit);
}

bool mysql_mgr::add_friend_apply(const int& from, const int& to)
{
	return _dao.add_friend_apply(from, to);
}

bool mysql_mgr::auth_friend_apply(const int& from, const int& to) {
	return _dao.auth_friend_apply(from, to);
}

bool mysql_mgr::add_friend(const int& from, const int& to, std::string back_name) {
	return _dao.add_friend(from, to, back_name);
}

bool mysql_mgr::get_friend_list(int self_id, std::vector<std::shared_ptr<user_info> >& user_info) {
	return _dao.get_friend_list(self_id, user_info);
}




