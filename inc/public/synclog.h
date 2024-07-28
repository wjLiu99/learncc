#ifndef __SYNC_LOG__
#define __SYNC_LOG__

#include <thread>
#include <condition_variable>
#include <mutex>

#include <iostream>
#include <any>
#include <queue>
#include <sstream>

// ANSI颜色码
#define ANSI_RESET   "\033[0m"
#define ANSI_RED     "\033[31m"
#define ANSI_GREEN   "\033[32m"
#define ANSI_YELLOW  "\033[33m"
#define ANSI_BLUE    "\033[34m"

#define ON      1
#define OFF     0

#define PRINT_ERROR     ON
#define PRINT_WARNING   ON
#define PRINT_INFO      ON
#define PRINT_DEBUG     ON



enum  _loglevel {
    DEBUG = 0,
    INFO = 1,
    WARNING = 2,
    ERROR = 3,
};

//异步日志任务
class log_task {
public:
    log_task(){}
    log_task(const log_task& src):level_(src.level_), log_data_(src.log_data_){}
    log_task(const log_task&& src):level_(src.level_),
        log_data_(std::move(src.log_data_)){}
    _loglevel level_;
    std::queue<std::any> log_data_;
};


class sync_log {

public:
    
    static sync_log& get_instance(){
        static sync_log instance;
        return instance;
    }

    ~sync_log() {

        std::cout << "exit success" << std::endl;
    }

    template<typename T>
    std::any  to_any(const T& value) {
        return std::any(value);
    }

 

    //可变参数列表，异步写
    template<typename...  Args>
    void sync_write(_loglevel level , Args&&... args) {
        auto task = std::make_shared<log_task>();
        //折叠表达式依次将可变参数写入队列,C++17
        (task->log_data_.push(args), ...);
        task->level_ = level;
        process(task);
       

    }





private:

    sync_log() {};

    sync_log& operator =(const sync_log&) = delete;
    sync_log(const sync_log&) = delete;

    //转换成字符串
    bool to_str(const std::any & data,  std::string& str) {
        std::ostringstream ss;
        if (data.type() == typeid(int)) {
            ss << std::any_cast<int>(data);
        } else if (data.type() == typeid(uint32_t)) {
            ss << std::any_cast<uint32_t>(data);

        } else if (data.type() == typeid(uint16_t)) {
            ss << std::any_cast<uint16_t>(data);
        }
        else if (data.type() == typeid(float)) {
            ss << std::any_cast<float>(data);
        }
        else if (data.type() == typeid(double)) {
            ss << std::any_cast<double>(data);
        }
        else if (data.type() == typeid(std::string)) {
            ss << std::any_cast<std::string>(data);
        }
        else if (data.type() == typeid(char*)) {
            ss << std::any_cast<char*>(data);
        }
        else if (data.type() == typeid(char const*)) {
            ss << std::any_cast<char const*>(data);
        }
        else {
            return false;
        }
        str = ss.str();
        return true;
    }
    //格式化输出日志任务，可调用第三方日志库
    void process(std::shared_ptr<log_task> task){
       

        if(task->log_data_.empty()){
            return;
        }
        // 队列首元素
        auto head = task->log_data_.front();
        task->log_data_.pop();

        std::string formatstr = "";
        bool bsuccess = to_str(head, formatstr);
        if (!bsuccess) {
            return;
        }
    
        for(; !(task->log_data_.empty()); ){
            auto data = task->log_data_.front();
            formatstr=fmt_str(formatstr, data);
            task->log_data_.pop();
        }


        switch (task->level_) {
        case ERROR:
            std::cout << ANSI_RED << "[ERROR] " << formatstr << ANSI_RESET << std::endl;
            break;
        case WARNING:
            std::cout << ANSI_YELLOW << "[WARNING] " << formatstr << ANSI_RESET << std::endl;
            break;
        case INFO:
            std::cout << "[INFO] " << formatstr <<  std::endl;
            break;
        case DEBUG:
            std::cout << ANSI_GREEN << "[DEBUG] " << formatstr << ANSI_RESET << std::endl;
            break;
        default:
            std::cout << "[UNKNOWN] " << formatstr << std::endl;
            break;
        }
        
       
    }
    template<typename ...Args>
    std::string fmt_str(const std::string& format, Args... args) {
            std::string result = format;
            size_t pos = 0;
            //lambda表达式查找并替换字符串，只替换第一个
            auto replace_placeholder = [&](const std::string& placeholder, const std::any& replacement) {
                std::string str_replement = "";
                bool bsuccess =  to_str(replacement, str_replement);
                if (!bsuccess) {
                    return;
                }
                
                size_t placeholderPos = result.find(placeholder, pos);
                if (placeholderPos != std::string::npos) {
                        result.replace(placeholderPos, placeholder.length(), str_replement);
                        pos = placeholderPos + str_replement.length();
                    }else{
                        result = result + " " + str_replement;
                    }
                };
            //折叠表达式，递归替换{}
            (replace_placeholder("{}", args),...);
            return result;
        }

};


template<typename ... Args>
void   _log3(Args&&... args) {
    sync_log::get_instance().sync_write(ERROR, std::forward<Args>(args)...);
    }

template<typename ... Args>
    void  _log2(Args&&... args) {
    sync_log::get_instance().sync_write(WARNING, std::forward<Args>(args)...);
}

template<typename ... Args>
    void  _log1(Args&&... args) {
    sync_log::get_instance().sync_write(INFO, std::forward<Args>(args)...);
}

template<typename ... Args>
    void  _log0(Args&&... args) {
    sync_log::get_instance().sync_write(DEBUG, std::forward<Args>(args)...);
}


#if PRINT_DEBUG
#define LOG_DEBUG(level, ...)\
    if (level <= DEBUG) {\
        _log0(__VA_ARGS__);\
    }

#else
#define LOG_DEBUG(level, ...)
#endif

#if PRINT_INFO
#define LOG_INFO(level, ...)\
    if (level <= INFO) {\
        _log1(__VA_ARGS__);\
    }

#else
#define LOG_INFO(level, ...)
#endif

#if PRINT_WARNING
#define LOG_WARNING(level, ...)\
    if (level <= WARNING) {\
        _log2(__VA_ARGS__);\
    }
#else
#define LOG_WARNING(level, ...)
#endif

#if PRINT_ERROR
#define LOG_ERROR(level, ...)\
    if (level <= ERROR) {\
        _log3(__VA_ARGS__);\
    }
#else
#define LOG_ERROR(level, ...)
#endif


#endif
