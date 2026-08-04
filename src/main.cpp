
#include "../incl/Server.hpp"

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
   if (argc != 3)
      return (1); //errror mess?
   
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