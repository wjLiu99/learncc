#include <iostream>
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>
#include "comm.h"
#include "cserver.h"
#include "conf_mgr.h"
#include <hiredis/hiredis.h>
#include "mysql_mgr.h"

int main()
{


	conf_mgr &cfg_mgr = conf_mgr::get_instance();
	std::string gate_port_str = cfg_mgr["gate_server"]["port"];
	unsigned short gate_port = atoi(gate_port_str.c_str());
	try
	{
        
		// unsigned short port = static_cast<unsigned short>(8080);
		net::io_context ioc{ 1 };
		boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);
		signals.async_wait([&ioc](const boost::system::error_code& error, int signal_number) {

			if (error) {
				return;
			}
			ioc.stop();
			});
		std::make_shared<cserver>(ioc, gate_port)->start();
		std::cout << "Gate Server listen on port: " << gate_port << std::endl;
		ioc.run();
	}
	catch (std::exception const& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	
}

