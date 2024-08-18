#include "csession.h"
#include "cserver.h"
#include <iostream>
#include <sstream>
#include "logic_system.h"

csession::csession(boost::asio::io_context& io_context, cserver* server):
	socket_(io_context), server_(server), b_close_(false),_b_head_parse(false){
	boost::uuids::uuid  a_session_id = boost::uuids::random_generator()();
	session_id_ = boost::uuids::to_string(a_session_id);
	recv_head_node_ = make_shared<msg_node>(HEAD_TOTAL_LEN);
}
csession::~csession() {
	std::cout << "~csession destruct" << endl;
}

tcp::socket& csession::get_socket() {
	return socket_;
}

std::string& csession::get_session_id() {
	return session_id_;
}

void csession::set_user_id (int uid) {
	user_id_ = uid;
}
int csession::get_user_id () {
	return user_id_;
}

void csession::start(){
	async_read_head(HEAD_TOTAL_LEN);
}

void csession::send(std::string msg, short msgid) {
	std::lock_guard<std::mutex> lock(send_lock_);
	int send_que_size = send_que_.size();
	// 超过发送队列上限禁止发送
	if (send_que_size > MAX_SENDQUE) {
		std::cout << "session: " << session_id_ << " send que fulled, size is " << MAX_SENDQUE << endl;
		return;
	}
	// 将待发送的数据构造成发送节点放入待发送队列中
	send_que_.push(make_shared<send_node>(msg.c_str(), msg.length(), msgid));
	if (send_que_size > 0) {
		return;
	}
	// 取出队头节点等待发送
	auto& msgnode = send_que_.front();
	boost::asio::async_write(socket_, boost::asio::buffer(msgnode->data_, msgnode->_total_len),
		std::bind(&csession::handle_write, this, std::placeholders::_1, shared_self()));
}

void csession::send(char* msg, short max_length, short msgid) {
	std::lock_guard<std::mutex> lock(send_lock_);
	int send_que_size = send_que_.size();
	if (send_que_size > MAX_SENDQUE) {
		std::cout << "session: " << session_id_ << " send que fulled, size is " << MAX_SENDQUE << endl;
		return;
	}

	send_que_.push(make_shared<send_node>(msg, max_length, msgid));
	if (send_que_size>0) {
		return;
	}
	// 连续调用send可能导致同一节点发送两次，添加pending判断正在发送就只把新节点插入队尾，不启动发送了
	auto& msgnode = send_que_.front();
	boost::asio::async_write(socket_, boost::asio::buffer(msgnode->data_, msgnode->_total_len), 
		std::bind(&csession::handle_write, this, std::placeholders::_1, shared_self()));
}

void csession::close() {
	socket_.close();
	b_close_ = true;
}

std::shared_ptr<csession>csession::shared_self() {
	return shared_from_this();
}

void csession::async_read_body(int total_len)
{
	auto self = shared_from_this();
	async_read_full(total_len, [self, this, total_len](const boost::system::error_code& ec, std::size_t bytes_transfered) {
		try {
			if (ec) {
				std::cout << "handle read failed, error is " << ec.message() << endl;
				close();
				server_->clear_session(session_id_);
				return;
			}

			if (bytes_transfered < total_len) {
				std::cout << "read length not match, read [" << bytes_transfered << "] , total ["
					<< total_len<<"]" << endl;
				close();
				server_->clear_session(session_id_);
				return;
			}

			memcpy(recv_msg_node_->data_ , data_ , bytes_transfered);
			recv_msg_node_->_cur_len += bytes_transfered;
			recv_msg_node_->data_[recv_msg_node_->_total_len] = '\0';
			cout << "receive data is " << recv_msg_node_->data_ << endl;
			//此处将消息投递到逻辑队列中
			logic_system::get_instance()->post_msg(make_shared<logic_node>(shared_from_this(), recv_msg_node_));
			//继续监听头部接受事件
			async_read_head(HEAD_TOTAL_LEN);
		}
		catch (std::exception& e) {
			std::cout << "Exception code is " << e.what() << endl;
		}
		});
}

void csession::async_read_head(int total_len)
{
	auto self = shared_from_this();
	// 读取指定字节数后调用回调函数
	async_read_full(HEAD_TOTAL_LEN, [self, this](const boost::system::error_code& ec, std::size_t bytes_transfered) {
		try {
			if (ec) {
				std::cout << "handle read failed, error is " << ec.message() << endl;
				close();
				server_->clear_session(session_id_);
				return;
			}
			// 读取字节数不够，发生错误直接断开连接
			if (bytes_transfered < HEAD_TOTAL_LEN) {
				std::cout << "read length not match, read [" << bytes_transfered << "] , total ["
					<< HEAD_TOTAL_LEN << "]" << endl;
				close();
				server_->clear_session(session_id_);
				return;
			}
			// 清空头部节点，将读取的数据拷贝进去
			recv_head_node_->clear();
			memcpy(recv_head_node_->data_, data_, bytes_transfered);

			//获取头部MSGID数据
			short msg_id = 0;
			memcpy(&msg_id, recv_head_node_->data_, HEAD_ID_LEN);
			//网络字节序转化为本地字节序
			msg_id = boost::asio::detail::socket_ops::network_to_host_short(msg_id);
			std::cout << "msg_id is " << msg_id << endl;
			//id非法
			if (msg_id > MAX_LENGTH) {
				std::cout << "invalid msg_id is " << msg_id << endl;
				server_->clear_session(session_id_);
				return;
			}
			short msg_len = 0;
			memcpy(&msg_len, recv_head_node_->data_ + HEAD_ID_LEN, HEAD_DATA_LEN);
			//网络字节序转化为本地字节序
			msg_len = boost::asio::detail::socket_ops::network_to_host_short(msg_len);
			std::cout << "msg_len is " << msg_len << endl;

			//id非法
			if (msg_len > MAX_LENGTH) {
				std::cout << "invalid data length is " << msg_len << endl;
				server_->clear_session(session_id_);
				return;
			}
			// 构造接收消息节点保存消息id交由logic system处理，开始读取消息体
			recv_msg_node_ = make_shared<recv_node>(msg_len, msg_id);
			async_read_body(msg_len);
		}
		catch (std::exception& e) {
			std::cout << "Exception code is " << e.what() << endl;
		}
		});
}

void csession::handle_write(const boost::system::error_code& error, std::shared_ptr<csession> shared_self) {
	//增加异常处理
	try {
		if (!error) {
			// 发送成功，队首节点出队，队列非空继续发送
			std::lock_guard<std::mutex> lock(send_lock_);
			//cout << "send data " << send_que_.front()->data_+HEAD_LENGTH << endl;
			send_que_.pop();
			if (!send_que_.empty()) {
				auto& msgnode = send_que_.front();
				boost::asio::async_write(socket_, boost::asio::buffer(msgnode->data_, msgnode->_total_len),
					std::bind(&csession::handle_write, this, std::placeholders::_1, shared_self));
			}
		}
		else {
			
			std::cout << "handle write failed, error is " << error.message() << endl;
			close();
			server_->clear_session(session_id_);
		}
	}
	catch (std::exception& e) {
		std::cerr << "Exception code : " << e.what() << endl;
	}
	
}

//读取完整长度
void csession::async_read_full(std::size_t maxLength, std::function<void(const boost::system::error_code&, std::size_t)> handler )
{
	::memset(data_, 0, MAX_LENGTH);
	async_read_len(0, maxLength, handler);
}

// 读取指定字节数                   
void csession::async_read_len(std::size_t read_len, std::size_t total_len, 
	std::function<void(const boost::system::error_code&, std::size_t)> handler)
{
	auto self = shared_from_this();
	socket_.async_read_some(boost::asio::buffer(data_ + read_len, total_len - read_len),
		[read_len, total_len, handler, self](const boost::system::error_code& ec, std::size_t  bytes_transfered) {
			if (ec) {
				// 出现错误，调用回调函数
				handler(ec, read_len + bytes_transfered);
				return;
			}

			if (read_len + bytes_transfered >= total_len) {
				//长度够了就调用回调函数
				handler(ec, read_len + bytes_transfered);
				return;
			}

			// 没有错误，且长度不足则继续读取
			self->async_read_len(read_len + bytes_transfered, total_len, handler);
	});
}

logic_node::logic_node(shared_ptr<csession>  session, 
	shared_ptr<recv_node> recvnode):session_(session),recv_node_(recvnode) {
	
}
