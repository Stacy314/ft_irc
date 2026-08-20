#pragma once

#include <sys/socket.h>
#include <csignal>
#include <cstring>
#include <netinet/in.h>
#include <unistd.h>
#include <cstdlib>
#include <stdlib.h>
#include <poll.h>
#include <vector>
#include <map>
#include "Utils.hpp"
#include <utility>
#include <string>
#include <sstream>
#include <iostream>
#include <fcntl.h>

#include "Utils.hpp"
#include "Client.hpp"
#include "Channel.hpp"


# define KNRM  "\x1B[0m"
# define KRED  "\x1B[31m"
# define KGRN  "\x1B[32m"
# define KYEL  "\x1B[33m"
# define KBLU  "\x1B[34m"
# define KMAG  "\x1B[35m"
# define KCYN  "\x1B[36m"
# define KWHT  "\x1B[37m"


class Server
{
	private:
		size_t port;
		std::string password;
		int serverFd;
		bool runing;
		std::vector<pollfd> pollfds;
	std::map<int, Client> clients;
    std::string           _serverName;
	std::map<std::string, Channel> _channels;
	Server(const Server &other);
	Server &operator=(const Server &other);
	public:
		Server(size_t  port, const std::string &password);
		~Server();
		Server();
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
		Channel *findChannel(const std::string &name);
		Channel &createChannel(const std::string &name);
		const std::string &getPassword() const;
		const std::string &serverName() const;
		Client *findNick(const std::string &nickname);
		void queue(Client &client, const std::string &message);
};

