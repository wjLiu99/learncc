#include "http_conn.h"
#include "logic_system.h"
//char 转为16进制
unsigned char to_hex(unsigned char x)
{
    return  x > 9 ? x + 55 : x + 48;
}
//16进制转10进制
unsigned char from_hex(unsigned char x)
{
    unsigned char y;
    if (x >= 'A' && x <= 'Z') y = x - 'A' + 10;
    else if (x >= 'a' && x <= 'z') y = x - 'a' + 10;
    else if (x >= '0' && x <= '9') y = x - '0';
    else assert(0);
    return y;
}

std::string url_encode(const std::string& str)
{
    std::string strTemp = "";
    size_t length = str.length();
    for (size_t i = 0; i < length; i++)
    {
        //判断是否仅有数字和字母构成
        if (isalnum((unsigned char)str[i]) ||
            (str[i] == '-') ||
            (str[i] == '_') ||
            (str[i] == '.') ||
            (str[i] == '~'))
            strTemp += str[i];
        else if (str[i] == ' ') //为空字符
            strTemp += "+";
        else
        {
            //其他字符需要提前加%并且高四位和低四位分别转为16进制
            strTemp += '%';
            strTemp += to_hex((unsigned char)str[i] >> 4);
            strTemp += to_hex((unsigned char)str[i] & 0x0F);
        }
    }
    return strTemp;
}


std::string url_decode(const std::string& str)
{
    std::string strTemp = "";
    size_t length = str.length();
    for (size_t i = 0; i < length; i++)
    {
        //还原+为空
        if (str[i] == '+') strTemp += ' ';
        //遇到%将后面的两个字符从16进制转为char再拼接
        else if (str[i] == '%')
        {
            assert(i + 2 < length);
            unsigned char high = from_hex((unsigned char)str[++i]);
            unsigned char low = from_hex((unsigned char)str[++i]);
            strTemp += high * 16 + low;
        }
        else strTemp += str[i];
    }
    return strTemp;
}


void http_conn::parse_getparam() {
    // 提取 URI  
    auto uri = req_.target().to_string();
    // 查找查询字符串的开始位置（即 '?' 的位置）  
    auto query_pos = uri.find('?');
    if (query_pos == std::string::npos) {
        get_url_ = uri;
        return;
    }
    get_url_ = uri.substr(0, query_pos);
    std::string query_string = uri.substr(query_pos + 1);
    std::string key;
    std::string value;
    size_t pos = 0;
    while ((pos = query_string.find('&')) != std::string::npos) {
        auto pair = query_string.substr(0, pos);
        size_t eq_pos = pair.find('=');
        if (eq_pos != std::string::npos) {
            key = url_decode(pair.substr(0, eq_pos)); // 假设有 url_decode 函数来处理URL解码  
            value = url_decode(pair.substr(eq_pos + 1));
            get_params_[key] = value;
        }
        query_string.erase(0, pos + 1);
    }
    // 处理最后一个参数对（如果没有 & 分隔符）  
    if (!query_string.empty()) {
        size_t eq_pos = query_string.find('=');
        if (eq_pos != std::string::npos) {
            key = url_decode(query_string.substr(0, eq_pos));
            value = url_decode(query_string.substr(eq_pos + 1));
            get_params_[key] = value;
        }
    }
}


//socket只有移动构造
http_conn::http_conn (tcp::socket s) : socket_(std::move(s)){

} 

void http_conn::start () {
    auto self = shared_from_this();
    http::async_read(socket_, buffer_, req_, [self](beast::error_code err, std::size_t size){
        try {
            //err可以直接当成bool的变量使用？
            if (err) {
                std::cout << "http read err : " << err.message() << std::endl; 
                return;
            }
            boost::ignore_unused(size);
            self->req_handler();
            //启动超时
            self->check_tmo();
        } catch(std::exception &e) {
            std::cout << "exception :" << e.what() << std::endl;

        }
    });
}

void http_conn::req_handler() {
    res_.version(req_.version());
    res_.keep_alive(false);

    if (http::verb::get == req_.method()) {
        parse_getparam();
        bool success = logic_system::get_instance()->get_handler(get_url_, shared_from_this());
        if (!success) {
            res_.result(http::status::not_found);
            res_.set(http::field::content_type, "text/plain");
            beast::ostream(res_.body()) << "url not found\r\n";
            write_response();
            return;
        }
        res_.set(http::field::content_type, "text/html");
        res_.result(http::status::ok);
        res_.set(http::field::server, "gate_server");
        write_response();
        return;
    }

    if (http::verb::post == req_.method()) {
        
        bool success = logic_system::get_instance()->post_handler(req_.target().to_string(), shared_from_this());
        if (!success) {
            res_.result(http::status::not_found);
            res_.set(http::field::content_type, "text/plain");
            beast::ostream(res_.body()) << "url not found\r\n";
            write_response();
            return;
        }
        res_.set(http::field::content_type, "text/html");
        res_.result(http::status::ok);
        res_.set(http::field::server, "gate_server");
        write_response();
        return;
    }
}


void http_conn::write_response () {
    auto self = shared_from_this();
    res_.content_length(res_.body().size());
    http::async_write(socket_, res_, [self] (beast::error_code err, std::size_t size) {
        //写完直接关闭连接，取消定时器
        self->socket_.shutdown(tcp::socket::shutdown_send, err);
        self->tmo_.cancel();
    });
}

void http_conn::check_tmo () {
    auto self = shared_from_this();
    tmo_.async_wait([self] (beast::error_code err) {
        if (!err) {
            //主动关客户端会造成隐患，如果对端还活着就会timewait，对端已经关闭就不会
            self->socket_.close(err);
        }
    });
}