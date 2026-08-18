#pragma once
#include <sys/socket.h>
#include <csignal>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstdlib>
#include <stdlib.h>
#include <poll.h>
#include <vector>
#include "../incl/Client.hpp"
#include <map>
#include <utility>

# define KNRM  "\x1B[0m"
# define KRED  "\x1B[31m"
# define KGRN  "\x1B[32m"
# define KYEL  "\x1B[33m"
# define KBLU  "\x1B[34m"
# define KMAG  "\x1B[35m"
# define KCYN  "\x1B[36m"
# define KWHT  "\x1B[37m"

#include <iostream>
#include <fcntl.h>
#include <sstream>
class Server
{
	private:
		size_t port;
		std::string password;
		int serverFd;
		bool runing;
		std::vector<pollfd> pollfds;
		std::map<int, Client> clients;
	public:
		Server();
		Server(size_t  port, const std::string &password);
		Server(const Server& obj);
		Server& operator=(const Server&  obj);
		~Server();

		int getPort();
		std::string getPassword();
		void setPort(size_t  port);
		void setPassword(std::string password);
		void start();
		void createSocket();
		void bindSocket();
		void listenSocket();
		void pollLoop();
		void addPollfd(int fd);
		void	acceptClient();
		void	reciveCom(int fd);


};