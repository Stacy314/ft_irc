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
#include <utility>
#include <string>
#include <sstream>
#include <iostream>
#include <fcntl.h>

#include "CommandUtils.hpp"
#include "Client.hpp"
#include "Channel.hpp"
#include "CommandHandler.hpp"
#include "Parser.hpp"

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
    int fd;
    size_t port;
    std::string password;
    int serverFd;
    bool runing;
    std::vector<pollfd> pollfds;
    std::map<int, Client> clients;
    std::string _serverName;
    std::map<std::string, Channel> _channels;
    CommandHandler _handler;
    Server(const Server &other);
    Server &operator=(const Server &other);

public:
    Server(size_t port, const std::string &password);
    Server();
    ~Server();
    int getPort();
    void setPort(size_t port);
    void setPassword(std::string password);
    void start();
    void createSocket();
    void bindSocket();
    void listenSocket();
    void pollLoop();
    void addPollfd(int fd);
    void acceptClient();
    void reciveCom(int fd);
    void disconnectClient(Client &client);
    void removeChannel(const std::string &name);
    void updatePollEvents(int fd);
    void flushClient(int fd);
    Channel *findChannel(const std::string &name);
    Channel &createChannel(const std::string &name);
    Client *findNick(const std::string &nickname);
    void queue(Client &client, const std::string &message);
    const std::string &getPassword() const;
    const std::string &serverName() const;
	void broadcastQuit(Client &client, const std::string &message);
    void broadcastNickChange(Client &client, const std::string &message);
    std::map<std::string, Channel> &getChannels();
};

