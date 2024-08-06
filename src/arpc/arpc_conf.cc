#include "arpc_conf.h"

#include "synclog.h"
#include <string>

void arpc_conf::load_config_file (const char *filename) {
    FILE *f = fopen(filename, "r");
    if (nullptr == f) {
        LOG_ERROR(LOG_LEVEL, filename, "is not exist!");
        exit(EXIT_FAILURE);
    }

    while (!feof(f)) {
        char buf[512] = {0};
        fgets(buf, 512, f);
        std::string read_buf(buf);
        trim(read_buf);

        if (read_buf[0] == '#' || read_buf.empty()) {
            continue;
        }

        int idx = read_buf.find('=');
        if (idx == -1) {
            continue;
        }
        int end = read_buf.find('\n', idx);

        std::string key, value;
        key = read_buf.substr(0, idx);
        trim(key);
        value = read_buf.substr(idx + 1, end - idx - 1);
        trim(value);
        conf_map_.insert({key, value});

    }
    fclose(f);
}

std::string arpc_conf::load(const std::string &key)
{
    auto it = conf_map_.find(key);
    if (conf_map_.end() == it)
    {
        return "";
    }
    return it->second;
}



// 去掉字符串前后的空格
void arpc_conf::trim(std::string &src)
{
    int idx = src.find_first_not_of(' ');
    if (idx > 0)
    {
        // 说明字符串前面有空格
        src = src.substr(idx, src.size()-idx);
    }
    // 去掉字符串后面多余的空格
    idx = src.find_last_not_of(' ');
    if (idx < src.size() - 1)
    {
        // 说明字符串后面有空格
        src = src.substr(0, idx + 1);
    }
}
