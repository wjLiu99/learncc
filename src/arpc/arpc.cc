#include "arpc.h"
#include "synclog.h"
#include <unistd.h>
#include <string>
arpc_conf arpc::conf_;

void show_help () {
    LOG_INFO(LOG_LEVEL, "command -i <configfile>");
}
// 读配置文件
void arpc::init (int argc, char **argv) {
    if (argc < 2) {
        show_help();
        exit(EXIT_FAILURE);
    }

    int c = 0;
    std::string conf_file;
    while((c = getopt(argc, argv, "i:")) != -1)
    {
        switch (c)
        {
        case 'i':
            conf_file = optarg;
            break;
        case '?':
            show_help();
            exit(EXIT_FAILURE);
        case ':':
            show_help();
            exit(EXIT_FAILURE);
        default:
            break;
        }
    }

    conf_.load_config_file(conf_file.c_str());
}

arpc &arpc::get_instance () {
    static arpc instance;
    return instance;
}

arpc_conf &arpc::get_conf () {
    return conf_;
}