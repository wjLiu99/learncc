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

#include <boost/property_tree/ptree.hpp>  
#include <boost/property_tree/ini_parser.hpp>  
#include <boost/filesystem.hpp>   
#include <fstream> 

namespace beast = boost::beast;         
namespace http = beast::http;           
namespace net = boost::asio;            
using tcp = boost::asio::ip::tcp; 

enum errcode {
    SUCCESS = 0,
    ERR_JSON  = -1,
    ERR_RPC = -2,
};

class conf_mgr;
extern conf_mgr cfg_mgr;
#endif