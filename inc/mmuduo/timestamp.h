#ifndef _TIMESTAMP_H_
#define _TIMESTAMP_H_

#include <iostream>
#include <string>

// 时间类
class timestamp
{
public:
    timestamp();
    explicit timestamp(int64_t microSecondsSinceEpoch);
    static timestamp now();
    std::string to_str() const;
private:
    int64_t microSecondsSinceEpoch_;
};
#endif