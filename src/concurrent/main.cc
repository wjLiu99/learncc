#include "synclog.h"
#define LOG_LEVEL DEBUG
int main(int argc, char const *argv[])
{
    LOG_ERROR(LOG_LEVEL, "hello {}", "world");
    LOG_WARNING(LOG_LEVEL, "hello {}", 5);
    LOG_DEBUG(LOG_LEVEL, "eee");
    LOG_INFO(LOG_LEVEL, "dddd");
    while(1);
    return 0;
}
