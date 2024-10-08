#ifndef _CONF_MGR_H
#define _CONF_MGR_H
#include "comm.h"

struct section_info {

    section_info () {}

    
    ~section_info () {section_data_.clear();}

    section_info (const section_info& src) {
        section_data_ = src.section_data_;
    }

    section_info & operator= (const section_info &src) {
        if (this == &src) {
            return *this;
        }
        section_data_ = src.section_data_;
        return *this;
    }

    std::string operator[] (const std::string &key) {
        if (section_data_.end() == section_data_.find(key)) {
            return "";
        }

        return section_data_[key];
    }



    std::map<std::string, std::string> section_data_;
};
//配置文件管理类
class conf_mgr {
public:
    ~conf_mgr (){ conf_map_.clear(); }
    section_info operator[] (const std::string &section) {
        if (conf_map_.end() == conf_map_.find(section)) {
            return section_info();
        }

        return conf_map_[section];
    }

    static conf_mgr& get_instance () {
        static conf_mgr instance_;
        return instance_;
    }

    conf_mgr (const conf_mgr &src) = delete;
    conf_mgr & operator= (const conf_mgr &src) = delete;
    

private:
    conf_mgr();
    std::map<std::string, section_info> conf_map_;
};

#endif