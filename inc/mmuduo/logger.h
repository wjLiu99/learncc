#ifndef _LOGGER_H_
#define _LOGGER_H_


#include <string>

#include "noncopyable.h"

// LOG_INFO("%s %d", arg1, arg2)
#if 0
#define LOG_INFO(logmsgFormat, ...) \
    do \
    { \
        logger &logger = logger::get_instance(); \
        logger.set_level(INFO); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
    } while(0) 
#else
#define LOG_INFO(logmsgFormat, ...)
#endif

#define LOG_ERROR(logmsgFormat, ...) \
    do \
    { \
        logger &logger = logger::get_instance(); \
        logger.set_level(ERROR); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
    } while(0) 

#define LOG_FATAL(logmsgFormat, ...) \
    do \
    { \
        logger &logger = logger::get_instance(); \
        logger.set_level(FATAL); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
        exit(-1); \
    } while(0) 

#ifdef MUDEBUG
#define LOG_DEBUG(logmsgFormat, ...) \
    do \
    { \
        logger &logger = logger::get_instance(); \
        logger.set_level(DEBUG); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
    } while(0) 
#else
    #define LOG_DEBUG(logmsgFormat, ...)
#endif

// 定义日志的级别  INFO  ERROR  FATAL  DEBUG 
enum LogLevel
{
    INFO,  // 普通信息
    ERROR, // 错误信息
    FATAL, // core信息
    DEBUG, // 调试信息
};

// 输出一个日志类
class logger : noncopyable
{
public:
    // 获取日志唯一的实例对象
    static logger& get_instance();
    // 设置日志级别
    void set_level(int level);
    // 写日志
    void log(std::string msg);
private:
    int log_level_;
};
#endif