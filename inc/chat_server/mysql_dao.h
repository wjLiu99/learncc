#ifndef _MYSQL_DAO_H_
#define _MYSQL_DAO_H_
#include "comm.h"
#include <thread>
#include <mysql_driver.h>
#include <mysql_connection.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/exception.h>
#include "data.h"
class SqlConnection {
public:
	SqlConnection(sql::Connection* con, int64_t lasttime):_con(con), _last_oper_time(lasttime){}
	std::unique_ptr<sql::Connection> _con;
    // 上一次操作的时间，需要使连接保活
	int64_t _last_oper_time;
};

class MySqlPool {
public:
	MySqlPool(const std::string& url, const std::string& user, const std::string& pass, const std::string& schema, int pool_size)
		: url_(url), user_(user), pass_(pass), schema_(schema), pool_size_(pool_size), b_stop_(false){
		try {
			for (int i = 0; i < pool_size_; ++i) {
				sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
				auto*  con = driver->connect(url_, user_, pass_);
				con->setSchema(schema_);
				// 获取当前时间戳
				auto currentTime = std::chrono::system_clock::now().time_since_epoch();
				// 将时间戳转换为秒
				long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(currentTime).count();
				pool_.push(std::make_unique<SqlConnection>(con, timestamp));
			}

			_check_thread = 	std::thread([this]() {
				while (!b_stop_) {
					checkConnection();
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
	void checkConnection() {
		std::lock_guard<std::mutex> guard(mutex_);
		int poolsize = pool_.size();
		// 获取当前时间戳
		auto currentTime = std::chrono::system_clock::now().time_since_epoch();
		// 将时间戳转换为秒
		long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(currentTime).count();
		for (int i = 0; i < poolsize; i++) {
			auto con = std::move(pool_.front());
			pool_.pop();
            // 不论后续代码在哪里返回都会将连接放回连接池里
			Defer defer([this, &con]() {
				pool_.push(std::move(con));
			});

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

	std::unique_ptr<SqlConnection> get_conn() {
		std::unique_lock<std::mutex> lock(mutex_);
		cond_.wait(lock, [this] { 
			if (b_stop_) {
				return true;
			}		
			return !pool_.empty(); });
		if (b_stop_) {
			return nullptr;
		}
		std::unique_ptr<SqlConnection> con(std::move(pool_.front()));
		pool_.pop();
		return con;
	}

	void return_conn(std::unique_ptr<SqlConnection> con) {
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

	~MySqlPool() {
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
	int pool_size_;
	std::queue<std::unique_ptr<SqlConnection>> pool_;   //连接队列
	std::mutex mutex_;
	std::condition_variable cond_;
	std::atomic<bool> b_stop_;
	std::thread _check_thread;  // 检测线程，如果连接超过一分钟没有使用就主动发送一次请求，保活
};



class MysqlDao
{
public:
	MysqlDao();
	~MysqlDao();
	int RegUser(const std::string& name, const std::string& email, const std::string& pwd);
	bool CheckEmail(const std::string& name, const std::string & email);
	bool UpdatePwd(const std::string& name, const std::string& newpwd);
	bool CheckPwd(const std::string& email, const std::string& pwd, user_info& userInfo);
	bool TestProcedure(const std::string& email, int& uid, std::string& name);
	std::shared_ptr<user_info> GetUser(int uid);
	bool auth_friend_apply(const int& from, const int& to);
	bool AddFriend(const int& from, const int& to, std::string back_name);
	bool add_friend_apply(const int& from, const int& to);
	std::shared_ptr<user_info> GetUser(std::string name);
	bool GetApplyList(int touid, std::vector<std::shared_ptr<apply_info>>& applyList, int offset, int limit );
	bool get_friend_list(int self_id, std::vector<std::shared_ptr<user_info> >& user_info);
private:
	std::unique_ptr<MySqlPool> pool_;
};


#endif