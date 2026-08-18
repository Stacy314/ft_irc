#include "../incl/Server.hpp"
#include "../incl/Utils.hpp"

Server::Server(int port, const std::string &password) : _password(password), _serverName("ircserv.local") {
    (void)port;
}

Server::~Server() {}

const std::string &Server::getPassword() const {
    return _password;
}


const std::string &Server::serverName() const {
    return _serverName;
}

Channel *Server::findChannel(const std::string &name)
{
    std::map<std::string, Channel>::iterator it =
        _channels.find(name);

    if (it == _channels.end())
        return NULL;

    return &(it->second);
}
Channel &Server::createChannel(const std::string &name) {
    std::pair<
        std::map<std::string, Channel>::iterator,
        bool
    > result = _channels.insert(
        std::make_pair(name, Channel(name))
    );

    return result.first->second;
}

Client *Server::findNick(const std::string &nickname) {
    std::map<int, Client>::iterator it;

    for (it = _clients.begin(); it != _clients.end(); ++it) {
        if (toUpper(it->second.getNickname())
            == toUpper(nickname)) {
            return &it->second;
        }
    }
    return NULL;
}

void Server::queue(
    Client &client,
    const std::string &message)
{
    client.outbuf += message;
}