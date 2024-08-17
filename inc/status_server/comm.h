#ifndef _COMM_H_
#define _COMM_H_

#include <memory>
#include <iostream>
#include <map>
#include <functional>
#include "singleton.h"
#include <unordered_map>
#include <vector>
#include <fstream> 
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <queue>
#include "conf_mgr.h"
#include <cassert>
#include <string>
#include <cstring>
#include <sstream>



 

enum errcode {
    SUCCESS = 0,
    ERR_JSON  = -1,
    ERR_RPC = -2,
	ERR_VER_EXPIRED = -3, //验证码过期
	ERR_VER_CODE = -4, //验证码错误
	ERR_EXIST = -5,       //用户已经存在

    ERR_PWD = -6,    //密码错误
	ERR_EMAIL = -7,  //邮箱不匹配
	ERR_PWD_UPDATE = -8,  //更新密码失败
	ERR_PWD_INVAILD = -9,   //密码更新失败
	ERR_TOKEN = -10,   //Token错误
	ERR_UID = -11,  //uid无效


};


// Defer类
class Defer {
public:
	// 接受一个lambda表达式或者函数指针
	Defer(std::function<void()> func) : func_(func) {}

	// 析构函数中执行传入的函数
	~Defer() {
		func_();
	}

private:
	std::function<void()> func_;
};
#define CODEPREFIX "code_"
#define USERIPPREFIX  "uip_"
#define USERTOKENPREFIX  "utoken_"
#define IPCOUNTPREFIX  "ipcount_"
#define USER_BASE_INFO "ubaseinfo_"
#define LOGIN_COUNT  "logincount"

#endif