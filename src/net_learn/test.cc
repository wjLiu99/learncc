#include <iostream>
#include <boost/asio.hpp>
#include "session.h"
std::function<void()> f;
void test () {
	std::shared_ptr<int> shard1(new int(5));
	std::cout << shard1.use_count() << std::endl;
	f = [shard1] () {
		std::cout << shard1.use_count() << std::endl;
	};
	std::cout << shard1.use_count() << std::endl;
}
int main()
{
	// try {
	// 	boost::asio::io_context ioc;
	// 	using namespace std;
	// 	Server s(ioc, 10086);
	// 	ioc.run();
	// }
	// catch (std::exception& e) {
	// 	std::cerr << "Exception: " << e.what() << "\n";
	// }
	// return 0;
	test();
	f();
}
