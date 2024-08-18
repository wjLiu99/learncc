#ifndef _DATA_H_
#define _DATA_H_

#include <string>
// 用户信息
struct user_info {
	user_info():name(""), pwd(""),uid(0),email(""),nick(""),desc(""),sex(0), icon(""), back("") {}
	std::string name;
	std::string pwd;
	int uid;
	std::string email;
	std::string nick;
	std::string desc;
	std::string icon;
	int sex;
	std::string back;
};
// 好友申请信息
struct apply_info {
	apply_info(int uid, std::string name, std::string desc,
		std::string icon, std::string nick, int sex, int status)
		:uid_(uid),name_(name),desc_(desc),
		icon_(icon),nick_(nick),sex_(sex),status_(status){}

	int uid_;
	std::string name_;
	std::string desc_;
	std::string icon_;
	std::string nick_;
	int sex_;
	int status_;
};

#endif