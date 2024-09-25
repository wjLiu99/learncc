#ifndef _COMM_H_
#define _COMM_H_

#include <memory>
#include <iostream>
#include <map>
#include <thread>
#include <functional>
#include "singleton.h"
#include <unordered_map>

#include <vector>
#include <fstream> 

#include <mutex>
#include <atomic>
#include <condition_variable>
#include <queue>
#include <sstream>
#include "conf_mgr.h"
#include <cassert>
#include <cstring>



#define CODEPREFIX "code_"

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
	ERR_TOKEN = -10,   //Token失效
	ERR_UID = -11,  //uid无效

	
};

// Defer类，函数无论在哪里返回时都要释放资源，归还连接或者发送数据
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

#define MAX_LENGTH  1024 * 2
//头部总长度
#define HEAD_TOTAL_LEN 4
//头部id长度
#define HEAD_ID_LEN 2
//头部数据长度
#define HEAD_DATA_LEN 2
#define MAX_RECVQUE  10000
#define MAX_SENDQUE 1000


enum MSG_IDS {
	MSG_CHAT_LOGIN = 1005, //用户登陆
	MSG_CHAT_LOGIN_RSP = 1006, //用户登陆回包
	ID_SEARCH_USER_REQ = 1007, //用户搜索请求
	ID_SEARCH_USER_RSP = 1008, //搜索用户回包
	ID_ADD_FRIEND_REQ = 1009, //申请添加好友请求
	ID_ADD_FRIEND_RSP  = 1010, //申请添加好友回复
	ID_NOTIFY_ADD_FRIEND_REQ = 1011,  //通知用户添加好友申请
	ID_AUTH_FRIEND_REQ = 1013,  //认证好友请求
	ID_AUTH_FRIEND_RSP = 1014,  //认证好友回复
	ID_NOTIFY_AUTH_FRIEND_REQ = 1015, //通知用户认证好友申请

	ID_TEXT_CHAT_MSG_REQ = 1017, //文本聊天信息请求
	ID_TEXT_CHAT_MSG_RSP = 1018, //文本聊天信息回复
	ID_NOTIFY_TEXT_CHAT_MSG_REQ = 1019, //通知用户文本聊天信息
};

// redis中USERIPPREFIX + uid和server name的映射关系，登入时存入
#define USERIPPREFIX  "uip_"
// 存储 USERTOKENPREFIX + uid和token的对应关系，登入时由状态服务器存入redis，聊天服务器检查登入时token是否正确
#define USERTOKENPREFIX  "utoken_"

#define IPCOUNTPREFIX  "ipcount_"
// 通过uid查询user的基本信息
#define USER_BASE_INFO "ubaseinfo_"
// 通过server name 查询登入用户数量
#define LOGIN_COUNT  "logincount"
// 通过name查询user的基本信息
#define NAME_INFO  "nameinfo_"

#endif