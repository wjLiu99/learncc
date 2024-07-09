#ifndef _COMM_H_
#define _COMM_H_
#include <boost/beast/http.hpp>
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <memory>
#include <iostream>
#include <map>
#include <functional>
#include "singleton.h"
#include <unordered_map>
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>
#include <vector>

#include <boost/property_tree/ptree.hpp>  
#include <boost/property_tree/ini_parser.hpp>  
#include <boost/filesystem.hpp>   
#include <fstream> 

#include <mutex>
#include <atomic>
#include <condition_variable>
#include <queue>

#include "conf_mgr.h"
#include <cassert>
#include <hiredis/hiredis.h>


#define CODEPREFIX "code_"
namespace beast = boost::beast;         
namespace http = beast::http;           
namespace net = boost::asio;            
using tcp = boost::asio::ip::tcp; 

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

#endif