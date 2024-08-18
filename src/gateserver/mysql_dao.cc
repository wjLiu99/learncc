#include "mysql_dao.h"
#include "conf_mgr.h"
using namespace std;
mysql_dao::mysql_dao()
{
	auto & cfg = conf_mgr::get_instance();
	const auto& host = cfg["mysql"]["host"];
	const auto& port = cfg["mysql"]["port"];
	const auto& pwd = cfg["mysql"]["password"];
	const auto& schema = cfg["mysql"]["schema"];
	const auto& user = cfg["mysql"]["user"];
    // unique ptr初始化只能在初始化列表或者reset
	pool_.reset(new mysql_pool(host+":"+port, user, pwd,schema, 5));
}

mysql_dao::~mysql_dao(){
	pool_->close();
}

int mysql_dao::reg_user(const std::string& name, const std::string& email, const std::string& pwd)
{
	auto con = pool_->get_conn();
	try {
		if (con == nullptr) {
			return false;
		}
		// 准备调用存储过程
		unique_ptr < sql::PreparedStatement > stmt(con->_con->prepareStatement("CALL reg_user(?,?,?,@result)"));
		// 设置输入参数s
		stmt->setString(1, name);
		stmt->setString(2, email);
		stmt->setString(3, pwd);

		// 由于PreparedStatement不直接支持注册输出参数，我们需要使用会话变量或其他方法来获取输出参数的值

		  // 执行存储过程
		stmt->execute();
		// 如果存储过程设置了会话变量或有其他方式获取输出参数的值，你可以在这里执行SELECT查询来获取它们
	   // 例如，如果存储过程设置了一个会话变量@result来存储输出结果，可以这样获取：
	    unique_ptr<sql::Statement> stmtResult(con->_con->createStatement());
	    unique_ptr<sql::ResultSet> res(stmtResult->executeQuery("SELECT @result AS result"));
	    if (res->next()) {
	        int result = res->getInt("result");
            cout << "Result: " << result << endl;
            pool_->return_conn(std::move(con));
            return result;
	    }
	    pool_->return_conn(std::move(con));
		return -1;
	}
	catch (sql::SQLException& e) {
		pool_->return_conn(std::move(con));
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return -1;
	}
}

bool mysql_dao::check_email(const std::string& name, const std::string& email) {
	auto con = pool_->get_conn();
	try {
		if (con == nullptr) {
			return false;
		}

		// 准备查询语句
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("SELECT email FROM user WHERE name = ?"));

		// 绑定参数
		pstmt->setString(1, name);

		// 执行查询
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

		// 遍历结果集
		while (res->next()) {
			std::cout << "Check Email: " << res->getString("email") << std::endl;
			if (email != res->getString("email")) {
				pool_->return_conn(std::move(con));
				return false;
			}
			pool_->return_conn(std::move(con));
			return true;
		}
		return true;
	}
	catch (sql::SQLException& e) {
		pool_->return_conn(std::move(con));
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool mysql_dao::update_pwd(const std::string& name, const std::string& newpwd) {
	auto con = pool_->get_conn();
	try {
		if (con == nullptr) {
			return false;
		}

		// 准备查询语句
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("UPDATE user SET pwd = ? WHERE name = ?"));

		// 绑定参数
		pstmt->setString(2, name);
		pstmt->setString(1, newpwd);

		// 执行更新
		int updateCount = pstmt->executeUpdate();

		std::cout << "Updated rows: " << updateCount << std::endl;
		pool_->return_conn(std::move(con));
		return true;
	}
	catch (sql::SQLException& e) {
		pool_->return_conn(std::move(con));
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool mysql_dao::check_pwd(const std::string& email, const std::string& pwd, user_info& u_info) {
	auto con = pool_->get_conn();
	if (con == nullptr) {
		return false;
	}

	Defer defer([this, &con]() {
		pool_->return_conn(std::move(con));
		});

	try {
	

		// 准备SQL语句
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("SELECT * FROM user WHERE email = ?"));
		pstmt->setString(1, email); // 将username替换为你要查询的用户名

		// 执行查询
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		std::string origin_pwd = "";
		// 遍历结果集
		while (res->next()) {
			origin_pwd = res->getString("pwd");
			// 输出查询到的密码
			std::cout << "Password: " << origin_pwd << std::endl;
			break;
		}

		if (pwd != origin_pwd) {
			return false;
		}
		u_info.name = res->getString("name");
		u_info.email = email;
		u_info.uid = res->getInt("uid");
		u_info.pwd = origin_pwd;
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}
