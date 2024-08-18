#include "cserver.h"
#include <iostream>
#include "io_service_pool.h"
#include "user_mgr.h"
cserver::cserver(boost::asio::io_context& io_context, short port):io_context_(io_context), port_(port),
acceptor_(io_context, tcp::endpoint(tcp::v4(),port))
{
	cout << "Server start success, listen on port : " << port_ << endl;
	start_accept();
}

cserver::~cserver() {
	cout << "Server destruct listen on port : " << port_ << endl;
}

void cserver::handle_accept(shared_ptr<csession> new_session, const boost::system::error_code& error){
	if (!error) {
		// 监听新连接上的事件,开始读数据
		new_session->start();
		lock_guard<mutex> lock(mtx_);
		// 建立uuid和session的映射关系，session是一个uuid
		sessions_.insert(make_pair(new_session->get_session_id(), new_session));
	}
	else {
		cout << "session accept failed, error is " << error.message() << endl;
	
	}

	start_accept();
}

void cserver::start_accept() {
	// 从io pool中获取一个io context，使用该io context创建一个session，session中包含socketfd用于新连接， acceptor_接收到连接后就调用handle将新连接交给io context管理
	auto &io_context = io_service_pool::get_instance()->get_ioservice();
	shared_ptr<csession> new_session = make_shared<csession>(io_context, this);
	acceptor_.async_accept(new_session->get_socket(), std::bind(&cserver::handle_accept, this, new_session, placeholders::_1));
}

// 用户下线， 删除session， session在server和user mgr中管理， uuid是sessionid
void cserver::clear_session(std::string uuid) {

	if (sessions_.find(uuid) != sessions_.end()) {
		//移除用户uid和session的关联
		user_mgr::get_instance()->rm_user_session(sessions_[uuid]->get_user_id());
	}
	lock_guard<mutex> lock(mtx_);
	sessions_.erase(uuid);
}
