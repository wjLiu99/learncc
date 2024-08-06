#ifndef _ARPC_CONF_H_
#define _ARPC_CONF_H_

#include <map>
#include <string>

#define LOG_LEVEL INFO
class arpc_conf {
public:
    //加载配置文件
    void load_config_file (const char *file_name);
    //读取配置项
    std::string load (const std::string &key);

private:
    //保存配置项
    std::map<std::string, std::string> conf_map_;
    //去除字符串前后空格
    void trim (std::string &src);

};
#endif