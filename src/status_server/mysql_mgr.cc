#include "mysql_mgr.h"
using namespace std;

MysqlMgr::~MysqlMgr() {

}

int MysqlMgr::reg_user(const std::string& name, const std::string& email, const std::string& pwd)
{
	return _dao.reg_user(name, email, pwd);
}

bool MysqlMgr::check_email(const std::string& name, const std::string& email) {
	return _dao.check_email(name, email);
}

bool MysqlMgr::update_pwd(const std::string& name, const std::string& pwd) {
	return _dao.update_pwd(name, pwd);
}

MysqlMgr::MysqlMgr() {
}

bool MysqlMgr::check_pwd(const std::string& email, const std::string& pwd, UserInfo& userInfo) {
	return _dao.check_pwd(email, pwd, userInfo);
}



