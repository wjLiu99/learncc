#pragma once
#include "comm.h"
#include <thread>
#include <mysql_driver.h>
#include <mysql_connection.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/exception.h>

// Data Access Object  封装Mysql Connector C++库对MySQL数据库的基本操作  例如执行 SQL 查询、插入、更新或删除数据等
class sql_connection {
public:
	sql_connection(sql::Connection* con, int64_t lasttime):_con(con), _last_oper_time(lasttime){}
	std::unique_ptr<sql::Connection> _con;
    // 上一次操作的时间，需要使连接保活
	int64_t _last_oper_time;
};
// MySQL连接池
class mysql_pool {
public:
	mysql_pool(const std::string& url, const std::string& user, const std::string& pass, const std::string& schema, int poolSize)
		: url_(url), user_(user), pass_(pass), schema_(schema), poolSize_(poolSize), b_stop_(false){
		try {
			for (int i = 0; i < poolSize_; ++i) {
				sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
				// 连接数据库
				auto*  con = driver->connect(url_, user_, pass_);
				// 设置使用的数据库
				con->setSchema(schema_);
				// 获取当前时间戳
				auto current_time = std::chrono::system_clock::now().time_since_epoch();
				// 将时间戳转换为秒
				long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(current_time).count();
				// 构造sql_connection unique ptr进行管理
				pool_.push(std::make_unique<sql_connection>(con, timestamp));
			}

			_check_thread = 	std::thread([this]() {
				while (!b_stop_) {
					check_conn();
					std::this_thread::sleep_for(std::chrono::seconds(60));
				}
			});

			_check_thread.detach();
		}
		catch (sql::SQLException& e) {
			// 处理异常
			std::cout << "mysql pool init failed, error is " << e.what()<< std::endl;
		}
	}
    // 检测连接时长是否超时
	void check_conn() {
		std::lock_guard<std::mutex> guard(mutex_);
		int poolsize = pool_.size();
		// 获取当前时间戳
		auto current_time = std::chrono::system_clock::now().time_since_epoch();
		// 将时间戳转换为秒
		long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(current_time).count();
		for (int i = 0; i < poolsize; i++) {
			auto con = std::move(pool_.front());
			pool_.pop();
            // 不论后续代码在哪里返回都会将连接放回连接池里
			Defer defer([this, &con]() {
				pool_.push(std::move(con));
			});
			// 保活与数据库的连接，最少5s发起一次查询
			if (timestamp - con->_last_oper_time < 5) {
				continue;
			}
			
			try {
				std::unique_ptr<sql::Statement> stmt(con->_con->createStatement());
				stmt->executeQuery("SELECT 1");
				con->_last_oper_time = timestamp;
				//std::cout << "execute timer alive query , cur is " << timestamp << std::endl;
			}
			catch (sql::SQLException& e) {
				std::cout << "Error keeping connection alive: " << e.what() << std::endl;
				// 重新创建连接并替换旧的连接
				sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
				auto* newcon = driver->connect(url_, user_, pass_);
				newcon->setSchema(schema_);
				con->_con.reset(newcon);
				con->_last_oper_time = timestamp;
			}
		}
	}

	std::unique_ptr<sql_connection> get_conn() {
		std::unique_lock<std::mutex> lock(mutex_);
		cond_.wait(lock, [this] { 
			if (b_stop_) {
				return true;
			}		
			return !pool_.empty(); });
		if (b_stop_) {
			return nullptr;
		}
		std::unique_ptr<sql_connection> con(std::move(pool_.front()));
		pool_.pop();
		return con;
	}

	void return_conn(std::unique_ptr<sql_connection> con) {
		std::unique_lock<std::mutex> lock(mutex_);
		if (b_stop_) {
			return;
		}
		pool_.push(std::move(con));
		cond_.notify_one();
	}

	void close() {
		b_stop_ = true;
		cond_.notify_all();
	}

	~mysql_pool() {
		std::unique_lock<std::mutex> lock(mutex_);
		while (!pool_.empty()) {
			pool_.pop();
		}
	}

private:
	std::string url_;
	std::string user_;
	std::string pass_;
	std::string schema_;    // 数据库名称
	int poolSize_;
	std::queue<std::unique_ptr<sql_connection>> pool_;   //连接队列
	std::mutex mutex_;
	std::condition_variable cond_;
	std::atomic<bool> b_stop_;
	std::thread _check_thread;  // 检测线程，如果连接超过一分钟没有使用就主动发送一次请求，保活
};

struct UserInfo {
	std::string name;
	std::string pwd;
	int uid;
	std::string email;
};

class mysql_dao
{
public:
	mysql_dao();
	~mysql_dao();
	// 注册用户
	int reg_user(const std::string& name, const std::string& email, const std::string& pwd);
	// 根据name检查email是否正确
	bool check_email(const std::string& name, const std::string & email);
	// 更新密码
	bool update_pwd(const std::string& name, const std::string& newpwd);
	// 检查密码是否正确
	bool check_pwd(const std::string& email, const std::string& pwd, UserInfo& userInfo);
	
private:
	std::unique_ptr<mysql_pool> pool_;
};


