#include "logger.h"

#include "timestamp.h"

#include <iostream>

// 获取日志唯一的实例对象
logger& logger::get_instance()
{
    static logger logger;
    return logger;
}

// 设置日志级别
void logger::set_level(int level)
{
    log_level_ = level;
}

// 写日志  [级别信息] time : msg
void logger::log(std::string msg)
{
    switch (log_level_)
    {
    case INFO:
        std::cout << "[INFO]";
        break;
    case ERROR:
        std::cout << "[ERROR]";
        break;
    case FATAL:
        std::cout << "[FATAL]";
        break;
    case DEBUG:
        std::cout << "[DEBUG]";
        break;
    default:
        break;
    }

    // 打印时间和msg
    std::cout << timestamp::now().to_str() << " : " << msg << std::endl;
}