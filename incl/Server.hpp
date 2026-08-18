#pragma once

#include "../incl/Client.hpp"
#include "../incl/Channel.hpp"
#include <map>
#include <string>
#include <sstream>

class Server
{
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
