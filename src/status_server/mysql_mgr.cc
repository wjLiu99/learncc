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

bool mysql_mgr::check_pwd(const std::string& email, const std::string& pwd, UserInfo& userInfo) {
	return _dao.check_pwd(email, pwd, userInfo);
}



