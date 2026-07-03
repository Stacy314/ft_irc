#ifndef SERVER_HPP
#define SERVER_HPP

#include "Client.hpp"
#include "Channel.hpp"
#include <string>
#include <map>
#include <vector>
#include <poll.h>

class Server {
public:
    Server(int port, const std::string &password);
    ~Server();
    void run();

private:
    int _port;
    std::string _password;
    int _listenFd;
    bool _running;
    std::map<int, Client> _clients;
    std::map<std::string, Channel> _channels;
    std::vector<struct pollfd> _pfds;

    void setupSocket();
    void rebuildPollfds();
    void acceptClient();
    void readClient(int fd);
    void writeClient(int fd);
    void disconnectClient(int fd, const std::string &reason);
    void processLine(Client &c, const std::string &line);
    void tryRegister(Client &c);

    void queue(int fd, const std::string &msg);
    void numeric(Client &c, int code, const std::string &msg);
    void broadcast(Channel &ch, const std::string &msg, int exceptFd);
    void sendNames(Client &c, Channel &ch);
    Client *findNick(const std::string &nick);
    bool nickInUse(const std::string &nick) const;
    std::string serverName() const;

    void cmdPass(Client &c, const std::vector<std::string> &p);
    void cmdNick(Client &c, const std::vector<std::string> &p);
    void cmdUser(Client &c, const std::vector<std::string> &p);
    void cmdJoin(Client &c, const std::vector<std::string> &p);
    void cmdPrivmsg(Client &c, const std::vector<std::string> &p);
    void cmdPart(Client &c, const std::vector<std::string> &p);
    void cmdQuit(Client &c, const std::vector<std::string> &p);
    void cmdPing(Client &c, const std::vector<std::string> &p);
    void cmdTopic(Client &c, const std::vector<std::string> &p);
    void cmdKick(Client &c, const std::vector<std::string> &p);
    void cmdInvite(Client &c, const std::vector<std::string> &p);
    void cmdMode(Client &c, const std::vector<std::string> &p);
    void cmdCap(Client &c, const std::vector<std::string> &p);
    void cmdWho(Client &c, const std::vector<std::string> &p);
};

#endif
