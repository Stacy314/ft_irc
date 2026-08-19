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

#include "../incl/Client.hpp"
#include "../incl/Channel.hpp"
#include <map>
#include <string>
#include <sstream>

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
private:
    std::string           _password;
    std::string           _serverName;
    std::map<int, Client> _clients;
	std::map<std::string, Channel> _channels;
	Server(const Server &other);
	Server &operator=(const Server &other);

public:
    Server(int port, const std::string &password);
    ~Server();
	Channel *findChannel(const std::string &name);
	Channel &createChannel(const std::string &name);
    const std::string &getPassword() const;
    const std::string &serverName() const;
    Client *findNick(const std::string &nickname);
    void queue(Client &client, const std::string &message);
};
