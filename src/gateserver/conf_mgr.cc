#include "conf_mgr.h"

conf_mgr::conf_mgr () {
    boost::filesystem::path cur_path = boost::filesystem::current_path();
    boost::filesystem::path conf_path = cur_path / "conf.ini";
    std::cout << "conf path : " << conf_path << std::endl;

    boost::property_tree::ptree pt;
    boost::property_tree::read_ini(conf_path.string(), pt);


    for (const auto &section_pair : pt) {
        const std::string &sec_name = section_pair.first;
        const boost::property_tree::ptree &sec_tree = section_pair.second;

        std::map<std::string, std::string> sec_conf;
        for (const auto &kv : sec_tree) {
            const std::string &key = kv.first;
            const std::string &value = kv.second.get_value<std::string>();
            sec_conf[key] = value;
        }

        section_info sec_info;
        sec_info.section_data_ = sec_conf;
        conf_map_[sec_name] = sec_info;
    }

    //遍历输出所有的配置文件键值对
    for (const auto& section_entry : conf_map_) {
        const std::string& section_name = section_entry.first;
        section_info section_config = section_entry.second;
        std::cout << "[" << section_name << "]" << std::endl;
        for (const auto& key_value_pair : section_config.section_data_) {
            std::cout << key_value_pair.first << "=" << key_value_pair.second << std::endl;
        }
    }
}