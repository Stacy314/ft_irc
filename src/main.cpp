
#include "../incl/Server.hpp"
#include "../incl/Utils.hpp"
#include "../incl/Parser.hpp"
#include "../incl/Command.hpp"
#include "../incl/Client.hpp"
#include "../incl/CommandHandler.hpp"
#include <iostream>
#include <cstdlib>
#include <stdexcept>

#include "../incl/CommandTests.hpp"
#include "../incl/ChannelTests.hpp"


size_t parse_port(const std::string& str)
{
   std::stringstream ss(str);

   size_t  port = 0;
   char extra;

	if (!(ss >> port) || (ss >> extra))
		throw std::invalid_argument("Invalid port");
	if (port < 1024 || port > 65535)
		throw std::invalid_argument("Port must be between 1024 and 65535");
	return (port);
}

int main (int argc, char **argv)
{
   // runChannelTests();
   if (argc != 3){
      std::cerr << "Usage: " << argv[0] << " <port> <password>\n";
      return (1);
   }
   
   try
   {
		Server server(parse_port(argv[1]), argv[2]);
		server.start();
   }
   catch(const std::exception& e)
   {
      std::cerr << KRED << e.what() << KNRM <<  '\n';
      return (1); 
   }
   
   return (0); 
}